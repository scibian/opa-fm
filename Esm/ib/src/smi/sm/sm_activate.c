/* BEGIN_ICS_COPYRIGHT7 ****************************************

Copyright (c) 2015-2017, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

** END_ICS_COPYRIGHT7   ****************************************/

/* [ICS VERSION STRING: unknown] */

#include "sm_l.h"

static __inline__ int
needs_reregistration(Node_t * nodep, Port_t * portp, pActivationRetry_t retry)
{
	if (activation_retry_attempts(retry)) {
		// on retries, only reregister if pending
		return portp->portData->reregisterPending;
	}

	return topology_passcount < 1 // new sm
		|| sm_find_guid(&old_topology, nodep->nodeInfo.NodeGUID) == NULL // new node
		|| portp->portData->reregisterPending; // previous attempts haven't yet succeeded
}

static void
handle_activate_failure(Port_t * portp)
{
	// limit resweeps, if a device consistently can't be activated, it may
	// have a HW or config issue
	if (++portp->portData->numFailedActivate <= ACTIVATE_RETRY_LIMIT)
		sm_request_resweep(1, 1, SM_SWEEP_REASON_ACTIVATE_FAIL);
}

static void
handle_activate_bounce(Node_t * nodep, Port_t * portp)
{
	if(sm_config.portBounceLogLimit == PORT_BOUNCE_LOG_NO_LIMIT ||
		topology_port_bounce_log_num < sm_config.portBounceLogLimit) {

		IB_LOG_WARN_FMT(__func__, "Arm/activate port for node %s guid " FMT_U64
			" port %d: port has bounced since programming start, requesting resweep",
			sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, portp->index);

		topology_port_bounce_log_num++;

		if(topology_port_bounce_log_num == sm_config.portBounceLogLimit) {
			IB_LOG_WARN_FMT(__func__,
				"Port bounce log limit of %d reached. Further port bounce events will be suppressed.",
				sm_config.portBounceLogLimit);
		}
	}

	sm_request_resweep(0, 0, SM_SWEEP_REASON_UNEXPECTED_BOUNCE);
}

static void
handle_activate_retry(Node_t * nodep, Port_t * portp, pActivationRetry_t retry)
{
	activation_retry_inc_failures(retry);

	if(activation_retry_attempts(retry) == sm_config.neighborNormalRetries) {
		IB_LOG_ERROR_FMT(__func__,
			"Port will not go active for node %s nodeGuid "FMT_U64" port %d after %d retries",
			sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, portp->index, activation_retry_attempts(retry));
		handle_activate_failure(portp);
	}
}

static Status_t
sm_arm_port(Topology_t * topop, Node_t * nodep, Port_t * portp)
{
	Status_t status = VSTATUS_OK;
	STL_PORT_INFO portInfo;
	STL_LID dlid;
	uint32_t madStatus = 0;

	IB_ENTER(__func__, topop, nodep, portp, 0);

	//do not try to activate ports connected to quarantined nodes
	if(portp->portData->neighborQuarantined) {
		IB_EXIT(__func__, status);
		return (status);
	}

	if (nodep->nodeInfo.NodeType == NI_TYPE_SWITCH) {
		Port_t *swportp;
		swportp = sm_get_port(nodep, 0);
		if (!sm_valid_port(swportp)) {
			IB_LOG_WARN_FMT(__func__, "Failed to get Port 0 of Switch " FMT_U64,
							nodep->nodeInfo.NodeGUID);
			return VSTATUS_BAD;
		}
		dlid = swportp->portData->lid;
	} else {
		dlid = portp->portData->lid;
	}

	portInfo = portp->portData->portInfo;

	if (portInfo.PortStates.s.PortState < IB_PORT_INIT) {
		IB_LOG_WARN_FMT(__func__, "Node %s guid "FMT_U64
			" port %u isn't in INIT state.",
			sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, portp->index);
		IB_EXIT(__func__, VSTATUS_BAD);
		return (VSTATUS_BAD);
	} else if (portInfo.PortStates.s.PortState > IB_PORT_INIT) {
		IB_LOG_INFINI_INFO_FMT(__func__, "Node %s guid "FMT_U64
			" port %u already armed or active.",
			sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, portp->index);
		IB_EXIT(__func__, VSTATUS_OK);
		return (VSTATUS_OK);
	}

	portInfo.PortStates.s.PortState = IB_PORT_ARMED;
	portInfo.LinkDownReason = STL_LINKDOWN_REASON_NONE;
	portInfo.NeighborLinkDownReason = STL_LINKDOWN_REASON_NONE;


	if (sm_is_scae_allowed(nodep))
		portInfo.PortMode.s.IsActiveOptimizeEnabled = 1;

	//
	//  Set the "No change" attributes.
	//
	portInfo.LinkSpeed.Enabled = 0;
	portInfo.LinkWidth.Enabled = 0;
	portInfo.PortStates.s.PortPhysicalState = 0;
	portInfo.s4.OperationalVL = 0;

	//
	//  Tell the port its new state.
	//
	status = SM_Set_PortInfo_LR(fd_topology, (1 << 24) | (portp->index),
		sm_lid, dlid, &portInfo, portp->portData->portInfo.M_Key, &madStatus);

	if (status != VSTATUS_OK && madStatus != MAD_STATUS_INVALID_ATTRIB) {
		IB_LOG_WARN_FMT(__func__,
			"Cannot set PORTINFO for node %s nodeGuid " FMT_U64
			" port %d status=%d", sm_nodeDescString(nodep),
			nodep->nodeInfo.NodeGUID, portp->index, status);

	} else if(madStatus == MAD_STATUS_INVALID_ATTRIB &&
		portInfo.PortStates.s.IsSMConfigurationStarted == 0) {
		handle_activate_bounce(nodep, portp);
	} else if (portInfo.PortStates.s.PortState != IB_PORT_ARMED &&
		!(madStatus == MAD_STATUS_INVALID_ATTRIB)) {

		IB_LOG_WARN_FMT(__func__,
						"Activate port for node %s guid " FMT_U64
						" port %d: tried to set state to %d but returned %d",
						sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, portp->index,
						IB_PORT_ARMED, portInfo.PortStates.s.PortState);
		// limit resweeps, if a device consistently can't be activated, it may
		// have a HW or config issue
		handle_activate_failure(portp);
	}

	// To save an additional Set(PortInfo) per port, the HOQLife/XmitQ values may not have been
	// sent for a port in topology_assignments() if at that time the port was not
	// ARMED or ACTIVE since at least one more Set(PortInfo) will be required to move
	// the port to ARMED.  Thus it is necessary to check the request/response
	// XmitQ values here as well
	if (status == VSTATUS_OK &&
		!sm_eq_XmitQ(portInfo.XmitQ, portp->portData->portInfo.XmitQ, STL_MAX_VLS)) {
		IB_LOG_ERROR_FMT(__func__,
			 "XmitQ requested/response value mismatch for node %s guid " FMT_U64
			 " port %d", sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID,
			 portp->index);
		status = VSTATUS_BAD;
	} else if (status == VSTATUS_OK) {
		//
		//  Save the port state and the port info
		//
		portp->state = portInfo.PortStates.s.PortState;
		portp->portData->portInfo = portInfo;

		// Clear the LED if it was previously turned on.
		sm_enable_port_led(nodep, portp, FALSE);
	}

	IB_EXIT(__func__, status);
	return (status);
}

static Port_t *
sm_arm_switch(Topology_t *topop, Node_t *switchp)
{
	Port_t *swportp, *portp;
	STL_LID dlid;
	const size_t blockSize = sizeof(STL_AGGREGATE) + 8*((sizeof(STL_PORT_INFO) + 7)/8) ;
	const size_t portsPerAggr = STL_MAX_PAYLOAD_SMP_LR / blockSize;
	uint8_t buffer[STL_MAX_PAYLOAD_SMP_LR] = {0};
	STL_AGGREGATE *aggrHdr = (STL_AGGREGATE*)buffer;
	uint8_t resetIndex = 0;
	uint8_t enqueued = 0;
	Status_t status;

	if (!sm_valid_port(swportp = sm_get_port(switchp, 0))) {
		IB_LOG_WARN_FMT(__func__, "Failed to get Port 0 of Switch " FMT_U64,
						switchp->nodeInfo.NodeGUID);
		return swportp;
	}
	dlid = swportp->portData->lid;

	for_all_ports(switchp, portp) {
		STL_PORT_INFO *pPortInfo = (STL_PORT_INFO*)aggrHdr->Data;

		if (sm_valid_port(portp) && !(portp->portData->neighborQuarantined)) {
			if (portp->state != IB_PORT_INIT) {
				if (portp->state == IB_PORT_DOWN && portp->portData->linkPolicyViolation) {
					//port marked down administratively for link policy violation but real port state
					//should still be in init, so set linkInitReason here
					sm_set_linkinit_reason(switchp, portp, STL_LINKINIT_OUTSIDE_POLICY);
					sm_enable_port_led(switchp, portp, TRUE);
				}

			} else {
				aggrHdr->AttributeID = STL_MCLASS_ATTRIB_ID_PORT_INFO;
				aggrHdr->Result.s.Error = 0;
				aggrHdr->Result.s.Reserved = 0;
				aggrHdr->Result.s.RequestLength = (sizeof(STL_PORT_INFO) + 7)/8;

				aggrHdr->AttributeModifier =  (1<<24) | (uint32_t)portp->index;

				*pPortInfo = portp->portData->portInfo;
				pPortInfo->PortStates.s.PortState = IB_PORT_ARMED;
				pPortInfo->LinkDownReason = STL_LINKDOWN_REASON_NONE;
				pPortInfo->NeighborLinkDownReason = STL_LINKDOWN_REASON_NONE;
				pPortInfo->PortStates.s.LEDEnabled = 0;

				if (sm_is_scae_allowed(switchp))
					pPortInfo->PortMode.s.IsActiveOptimizeEnabled = 1;

				// No-Op attributes
				pPortInfo->LinkSpeed.Enabled = 0;
				pPortInfo->LinkWidth.Enabled = 0;
				pPortInfo->PortStates.s.PortPhysicalState = 0;
				pPortInfo->s4.OperationalVL = 0;

				BSWAP_STL_PORT_INFO(pPortInfo);
				aggrHdr = STL_AGGREGATE_NEXT(aggrHdr);

				if (enqueued++ < 1)
					resetIndex = portp->index;
			}
		}

		// Send out the Aggregate.
		if ((enqueued > 0) &&
			(portp->index == switchp->nodeInfo.NumPorts || (enqueued == portsPerAggr))) {
			uint32_t madStatus = 0;
			STL_AGGREGATE *lastSeg = NULL;

			status = SM_Set_Aggregate_LR(fd_topology, (STL_AGGREGATE*)buffer,
				aggrHdr, sm_lid, dlid, sm_config.mkey, &lastSeg, &madStatus);

			// Process the results
			if (lastSeg) {
				for (aggrHdr = (STL_AGGREGATE*)buffer; aggrHdr < lastSeg; aggrHdr = STL_AGGREGATE_NEXT(aggrHdr)) {
					Port_t *retPort;

					retPort = sm_get_port(switchp, aggrHdr->AttributeModifier & 0xff);
					if (!retPort) {
						IB_LOG_ERROR_FMT(__func__, "Invalid port number in Set(PortInfo) response for node %s",
										sm_nodeDescString(switchp));
						continue;
					}

					pPortInfo = (STL_PORT_INFO*)aggrHdr->Data;

					// Aggregate fails, re-process this port the old fashion way.
					if (aggrHdr->Result.s.Error) {
						IB_LOG_ERROR_FMT(__func__, "Error on Set(Aggregate) of PortInfo for port %d of node %s",
										retPort->index, sm_nodeDescString(switchp));
						// All following Set(PortInfo) in the aggregate weren't processed. Rebuild aggregate from port following failure.
						portp = retPort;
						break;
					}
					BSWAP_STL_PORT_INFO(pPortInfo);


					// Some error cases that allow us to continue working on the Aggregate, but should be handled (taken from sm_activate_port)
					if (pPortInfo->PortStates.s.PortState != IB_PORT_ARMED && pPortInfo->PortStates.s.NeighborNormal != 0) {
						IB_LOG_WARN_FMT(__func__, "Arm port for node %s guid " FMT_U64" port %d failed",
										sm_nodeDescString(switchp), switchp->nodeInfo.NodeGUID, retPort->index);

						// limit resweeps, if a device consistently can't be activated, it may
						// have a HW or config issue
						if (++retPort->portData->numFailedActivate <= ACTIVATE_RETRY_LIMIT)
							sm_request_resweep(1, 1, SM_SWEEP_REASON_ACTIVATE_FAIL);

						continue;
					}

					if (!sm_eq_XmitQ(pPortInfo->XmitQ, retPort->portData->portInfo.XmitQ, STL_MAX_VLS)) {
						IB_LOG_ERROR_FMT(__func__,
										 "XmitQ requested/response value mismatch for node %s guid " FMT_U64
										 " port %d", sm_nodeDescString(switchp), switchp->nodeInfo.NodeGUID,
										 retPort->index);
						continue;
					}


					retPort->state = pPortInfo->PortStates.s.PortState;
					retPort->portData->portInfo = *pPortInfo;
					if (retPort->state == IB_PORT_ACTIVE)
						retPort->portData->numFailedActivate = 0;


					sm_enable_port_led(switchp, retPort, FALSE);
					bitset_clear(&switchp->initPorts, retPort->index);
				}
			} else {
				// Aggregate fell flat on it's face. Reset the port pointer, we'll build another one
				// from the next port.
				IB_LOG_ERROR_FMT(__func__, "Error on Set(Aggregate) for Node "FMT_U64 ":%s, status = %d",
								switchp->nodeInfo.NodeGUID, sm_nodeDescString(switchp), status);
				portp = sm_get_port(switchp, resetIndex);
			}

			// ARM this port manually if we had Aggregate troubles.
			if (!lastSeg || aggrHdr->Result.s.Error) {
				if (sm_valid_port(portp) && sm_arm_port(topop, switchp, portp) != VSTATUS_OK) {
					IB_LOG_ERROR_FMT(__func__, "TT(ta): can't ARM switch %s nodeGuid "FMT_U64" node index %d port index %d",
									sm_nodeDescString(switchp), switchp->nodeInfo.NodeGUID, switchp->index, portp->index);
					sm_enable_port_led(switchp, portp, TRUE);
				}
			}

			aggrHdr = (STL_AGGREGATE*)buffer;
			enqueued = 0;
		}
	}

	// NULL = Success
	return NULL;
}

Status_t
sm_activate_port(Topology_t * topop, Node_t * nodep, Port_t * portp,
	uint8_t forceReregister, pActivationRetry_t retry)
{
	Status_t status = VSTATUS_OK;
	STL_PORT_INFO portInfo;
	STL_LID dlid;
	uint32_t madStatus = 0;
	int reregisterable = TRUE;

	IB_ENTER(__func__, topop, nodep, portp, forceReregister);

	//do not try to activate ports connected to quarantined nodes
	if(portp->portData->neighborQuarantined)
		return VSTATUS_OK;

	if (nodep->nodeInfo.NodeType == NI_TYPE_SWITCH) {
		Port_t *swportp;
		swportp = sm_get_port(nodep, 0);
		if (!sm_valid_port(swportp)) {
			IB_LOG_WARN_FMT(__func__,
				"Failed to find node %s nodeGuid "FMT_U64" port 0",
				sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID);
			sm_enable_port_led(nodep, portp, TRUE);
			IB_EXIT(__func__, VSTATUS_BAD);
			return VSTATUS_BAD;
		}
		dlid = swportp->portData->lid;

		reregisterable = portp->index == 0 && nodep->switchInfo.u2.s.EnhancedPort0;
	} else {
		dlid = portp->portData->lid;
	}

	portInfo = portp->portData->portInfo;

	if (portInfo.PortStates.s.PortState < IB_PORT_ARMED) {
		IB_LOG_WARN_FMT(__func__,
			"Node %s nodeGuid "FMT_U64" port %u: state isn't ARMED or ACTIVE; state=%u",
			sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, portp->index, portInfo.PortStates.s.PortState);
		sm_enable_port_led(nodep, portp, TRUE);
		IB_EXIT(__func__, VSTATUS_BAD);
		return (VSTATUS_BAD);
	}

	if (portInfo.PortStates.s.PortState == IB_PORT_ACTIVE) {
		if (reregisterable && (needs_reregistration(nodep, portp, retry) || forceReregister)) {
			// In this case, the port is already active but we need to tell it to
			// re-register any multicast groups. This might be because the FM was
			// restarted or because the node just appeared in the fabric and
			// ActiveOptimize is enabled.
			portInfo.Subnet.ClientReregister = 1;
			// Indicate that reregistration is pending; will persist across
			// sweeps until it succeeds or is no longer required (e.g. state change)
			portp->portData->reregisterPending = 1;
			if (smDebugPerf) {
				IB_LOG_VERBOSE_FMT(__func__,
					"Setting ClientReregister bit in portInfo of node %s nodeGuid "FMT_U64" port %u",
					sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, portp->index);
			}
		} else {
			// If the port is already active and we don't need to re-register
			// MC groups, we don't need to actually send the MAD.
			portp->state = portInfo.PortStates.s.PortState;
			portp->portData->numFailedActivate = 0;
			IB_EXIT(__func__, VSTATUS_OK);
			return (VSTATUS_OK);
		}
	}

	portInfo.PortStates.s.PortState = IB_PORT_ACTIVE;

	//
	//  Set the "No change" attributes.
	//
	portInfo.LinkSpeed.Enabled = 0;
	portInfo.LinkWidth.Enabled = 0;
	portInfo.PortStates.s.PortPhysicalState = 0;
	portInfo.s4.OperationalVL = 0;

	//
	//  Tell the port its new state.
	//
	status = SM_Set_PortInfo_LR(fd_topology, (1 << 24) | (portp->index),
		sm_lid, dlid, &portInfo, portp->portData->portInfo.M_Key, &madStatus);

	if (status != VSTATUS_OK && madStatus != MAD_STATUS_INVALID_ATTRIB) {
		IB_LOG_WARN_FMT(__func__,
			"Cannot set PORTINFO for node %s nodeGuid "FMT_U64" port %d: status=%d madStatus=%u",
			sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, portp->index, status, madStatus);
		sm_enable_port_led(nodep, portp, TRUE);
		IB_EXIT(__func__, status);
		return (status);
	}

	// reset the pending reregistration flag as appropriate
	if (status == VSTATUS_OK || portInfo.PortStates.s.PortState != IB_PORT_ACTIVE)
		portp->portData->reregisterPending = 0;

	// check for failures to activate
	if (portInfo.PortStates.s.PortState != IB_PORT_ACTIVE) {
		if (madStatus == MAD_STATUS_INVALID_ATTRIB && !portInfo.PortStates.s.IsSMConfigurationStarted) {
			handle_activate_bounce(nodep, portp);
		} else if (madStatus == MAD_STATUS_INVALID_ATTRIB && portp->index > 0 && !portInfo.PortStates.s.NeighborNormal) {
			// check for obvious "neighbor normal not ready" failures
			handle_activate_retry(nodep, portp, retry);
		} else if (madStatus == MAD_STATUS_INVALID_ATTRIB && portp->index > 0
				&& !portp->portData->portInfo.PortStates.s.NeighborNormal
				&& portInfo.PortStates.s.NeighborNormal) {
			// we have observed NeighborNormal transitioning while the SMA
			// is processing the Set.  if NeighborNormal changed state during
			// this Set, conservatively assume it was a race and trigger a
			// retry.  if it fails on the retry, we'll fall through to the
			// general failure case.
			handle_activate_retry(nodep, portp, retry);
		} else {
			IB_LOG_WARN_FMT(__func__,
				"Activate port for node %s nodeGuid "FMT_U64" port %d: tried to set state to %d but returned %d",
				sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, portp->index,
				IB_PORT_ACTIVE, portInfo.PortStates.s.PortState);
			handle_activate_failure(portp);
		}
	}

	//
	//  Save the port state and the port info
	//
	portp->state = portInfo.PortStates.s.PortState;
	portp->portData->portInfo = portInfo;
	if (portp->state == IB_PORT_ACTIVE)
		portp->portData->numFailedActivate = 0;

	IB_EXIT(__func__, VSTATUS_OK);
	return (VSTATUS_OK);
}

static Status_t
activate_switch_via_portinfo(Topology_t * topop, Node_t * nodep, pActivationRetry_t retry)
{
	Status_t status;
	Port_t * portp;

	IB_ENTER(__func__, topop, nodep, retry, 0);

	for_all_physical_ports(nodep, portp) {
		if (!sm_valid_port(portp) || portp->state < IB_PORT_ARMED) continue;
		status = sm_activate_port(topop, nodep, portp, FALSE, retry);
		if (status != VSTATUS_OK) {
			IB_LOG_WARN_FMT(__func__,
				"Failed to activate node %s nodeGuid "FMT_U64" port 0; status=%u",
				sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, status);
			if ((status = sm_popo_port_error(&sm_popo, sm_topop, portp, status)) == VSTATUS_TIMEOUT_LIMIT)
				return status;
		}
	}

	IB_EXIT(__func__, VSTATUS_OK);
	return VSTATUS_OK;
}

static Status_t
activate_switch_via_portstateinfo(Topology_t * topop, Node_t * nodep, Port_t * port0p, pActivationRetry_t retry)
{
	Status_t status = VSTATUS_OK;
	int i;

	IB_ENTER(__func__, topop, nodep, retry, 0);

	STL_PORT_STATE_INFO * psi = NULL;
	status = sm_set_node_port_states(topop, nodep, port0p, NULL, IB_PORT_ARMED, IB_PORT_ACTIVE, &psi);
	if (status != VSTATUS_OK) {
		IB_LOG_WARN_FMT(__func__,
			"Failed Set(PortStateInfo) for node %s nodeGuid "FMT_U64": status=%u",
			sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, status);

		if ((status = sm_popo_port_error(&sm_popo, sm_topop, port0p, status)) == VSTATUS_TIMEOUT_LIMIT)
			return status;

		// if Set failed, see if we can Get and take advantage of any ports
		// that succeeded
		status = sm_get_node_port_states(topop, nodep, port0p, NULL, &psi);
		if (status != VSTATUS_OK) {
			IB_LOG_WARN_FMT(__func__,
				"Failed Get(PortStateInfo) for node %s nodeGuid "FMT_U64": status=%u",
				sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, status);

			if ((status = sm_popo_port_error(&sm_popo, sm_topop, port0p, status)) == VSTATUS_TIMEOUT_LIMIT)
				return status;

			// last resort... hit each port individually
			status = activate_switch_via_portinfo(topop, nodep, retry);
			IB_EXIT(__func__, status);
			return status;
		}
	}

	if (!psi) {
		// success and no buffer?  nothing needed to be done
		IB_EXIT(__func__, VSTATUS_OK);
		return VSTATUS_OK;
	}

	// we have a buffer: either Set succeeded, or it failed and we did a Get
	for(i = 0; i <= nodep->nodeInfo.NumPorts; i++) {
		Port_t * portp = sm_get_port(nodep, i);
		if (!sm_valid_port(portp) || portp->state < IB_PORT_ARMED) continue;

		portp->portData->portInfo.PortStates = psi[i].PortStates;
		portp->state = psi[i].PortStates.s.PortState;

		switch (psi[i].PortStates.s.PortState) {
			case IB_PORT_ACTIVE:
				// success
				portp->portData->numFailedActivate = 0;
				break;
			case IB_PORT_ARMED:
				// still armed or never got processed
				if (!psi[i].PortStates.s.IsSMConfigurationStarted) {
					handle_activate_bounce(nodep, portp);
				} else if (i > 0 && !psi[i].PortStates.s.NeighborNormal) {
					handle_activate_retry(nodep, portp, retry);
				} else {
					// looks okay; most likely that the Set(PortStateInfo) failed
					// before reaching this port, or that NeighborNormal propagated
					// between the set and get.  attempt directly via Set(PortInfo)
					status = sm_activate_port(topop, nodep, portp, FALSE, retry);
					if (status != VSTATUS_OK) {
						IB_LOG_WARN_FMT(__func__,
							"Failed to activate node %s nodeGuid "FMT_U64" port 0: status=%u",
							sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, status);
						if ((status = sm_popo_port_error(&sm_popo, sm_topop, portp, status)) == VSTATUS_TIMEOUT_LIMIT)
							break;
					}
				}
				break;
			default:
				// state < ARMED: port bounce
				handle_activate_bounce(nodep, portp);
				break;
		}
	}

	vs_pool_free(&sm_pool, psi);

	IB_EXIT(__func__, status);
	return status;
}

static Status_t
activate_switch(Topology_t * topop, Node_t * nodep, pActivationRetry_t retry)
{
	Status_t status;

	IB_ENTER(__func__, topop, nodep, retry, 0);

	Port_t *port0p = sm_get_port(nodep, 0);
	if (!sm_valid_port(port0p) || port0p->state < IB_PORT_INIT) {
		IB_LOG_WARN_FMT(__func__,
			"Switch %s nodeGuid "FMT_U64" has invalid port 0: unable to activate",
			sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID);

		// fail outright without access to port 0
		IB_EXIT(__func__, VSTATUS_BAD);
		return VSTATUS_BAD;
	}

	// activate port 0 via Set(PortInfo) if
	//   - it's enhanced, AND...
	//       - we're ARMED and retrying (as we only Set(PortStateInfo) on first attempt)
	//       - we're ACTIVE and need a reregistration
	if (  nodep->switchInfo.u2.s.EnhancedPort0
	   && (  (port0p->state == IB_PORT_ARMED && activation_retry_attempts(retry))
	      || (port0p->state == IB_PORT_ACTIVE && needs_reregistration(nodep, port0p, retry))))
	{
		status = sm_activate_port(topop, nodep, port0p, FALSE, retry);
		if (status != VSTATUS_OK) {
			IB_LOG_WARN_FMT(__func__,
				"Failed to activate node %s nodeGuid "FMT_U64" port 0: status=%u",
				sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, status);
			if ((status = sm_popo_port_error(&sm_popo, sm_topop, port0p, status)) == VSTATUS_TIMEOUT_LIMIT)
				return status;
		}
	}

	// for remaining switch ports, use PortStateInfo on the first attempt
	status = activation_retry_attempts(retry)
		? activate_switch_via_portinfo(topop, nodep, retry)
		: activate_switch_via_portstateinfo(topop, nodep, port0p, retry);

	IB_EXIT(__func__, status);
	return status;
}

static Status_t
activate_hfi(Topology_t * topop, Node_t * nodep, pActivationRetry_t retry)
{
	Status_t status;
	Port_t * portp;

	IB_ENTER(__func__, topop, nodep, retry, 0);

	for_all_ports(nodep, portp) {
		if (!sm_valid_port(portp) || portp->state < IB_PORT_ARMED) continue;
		status = sm_activate_port(topop, nodep, portp, FALSE, retry);
		if (status != VSTATUS_OK) {
			IB_LOG_WARN_FMT(__func__,
				"Failed to activate node %s nodeGuid "FMT_U64" port %u: status=%u",
				sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, portp->index, status);
			if ((status = sm_popo_port_error(&sm_popo, sm_topop, portp, status)) == VSTATUS_TIMEOUT_LIMIT)
				return status;
		}
	}

	IB_EXIT(__func__, VSTATUS_OK);
	return VSTATUS_OK;
}

// Activates a node "efficiently", e.g. leveraging PortStateInfo when available.
static Status_t
sm_activate_node(Topology_t * topop, Node_t * nodep, pActivationRetry_t retry)
{
	Status_t status = VSTATUS_OK;

	IB_ENTER(__func__, topop, nodep, retry, 0);

	if (nodep->nodeInfo.NodeType == NI_TYPE_SWITCH) {
		status = activate_switch(topop, nodep, retry);
	} else if (nodep->nodeInfo.NodeType == NI_TYPE_CA) {
		status = activate_hfi(topop, nodep, retry);
	}

	IB_EXIT(__func__, status);
	return status;
}

// Activates a node conservatively: one port at a time via PortInfo only.
static Status_t
sm_activate_node_safe(Topology_t * topop, Node_t * nodep, pActivationRetry_t retry)
{
	Status_t status = VSTATUS_OK;
	Port_t * portp;

	IB_ENTER(__func__, topop, nodep, retry, 0);

	for_all_ports(nodep, portp) {
		if (!sm_valid_port(portp) || portp->state < IB_PORT_ARMED) continue;
		status = sm_activate_port(sm_topop, nodep, portp, FALSE, retry);
		if (status != VSTATUS_OK) {
			IB_LOG_WARN_FMT(__func__,
				"Failed to activate node %s nodeGuid "FMT_U64" port %u; status=%u",
				sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, portp->index, status);
			if ((status = sm_popo_port_error(&sm_popo, sm_topop, portp, status)) == VSTATUS_TIMEOUT_LIMIT)
				return status;
		}
	}

	IB_EXIT(__func__, status);
	return status;
}

Status_t
sm_arm_node(Topology_t * topop, Node_t * nodep)
{
	Status_t status = VSTATUS_OK;
	Port_t *portp;

	if (nodep->nodeInfo.NodeType == NI_TYPE_SWITCH && sm_config.use_aggregates)  {
		sm_arm_switch(topop, nodep);
	} else {
		for_all_ports(nodep, portp) {
			if (sm_valid_port(portp) && portp->state == IB_PORT_INIT) {
				status = sm_arm_port(topop, nodep, portp);
				if (status != VSTATUS_OK) {
					IB_LOG_ERROR_FMT(__func__, "TT(ta): can't ARM node %s nodeGuid "FMT_U64" node index %d port index %d",
									sm_nodeDescString(nodep), nodep->nodeInfo.NodeGUID, nodep->index, portp->index);
					sm_enable_port_led(nodep, portp, TRUE);
					if ((status = sm_popo_port_error(&sm_popo, sm_topop, portp, status)) == VSTATUS_TIMEOUT_LIMIT)
						return status;
				} else {
					bitset_clear(&nodep->initPorts, portp->index);
				}
			} else if(sm_valid_port(portp) && portp->state == IB_PORT_DOWN && portp->portData->linkPolicyViolation) {
				//port marked down administratively for link policy violation but real port state
				//should still be in init, so set linkInitReason here
				sm_set_linkinit_reason(nodep, portp, STL_LINKINIT_OUTSIDE_POLICY);
				sm_enable_port_led(nodep, portp, TRUE);
			}
		}
	}

	return status;
}

Status_t
sm_activate_all_hfi_first_safe(Topology_t * topop, pActivationRetry_t retry)
{
	Status_t status = VSTATUS_OK;
	Node_t * nodep;

	for_all_ca_nodes(topop, nodep)
		if ((status = sm_activate_node_safe(topop, nodep, retry)) == VSTATUS_TIMEOUT_LIMIT)
			return status;
	for_all_switch_nodes(topop, nodep)
		if ((status = sm_activate_node_safe(topop, nodep, retry)) == VSTATUS_TIMEOUT_LIMIT)
			return status;

	return VSTATUS_OK;
}

Status_t
sm_activate_all_hfi_first(Topology_t * topop, pActivationRetry_t retry)
{
	Status_t status = VSTATUS_OK;
	Node_t * nodep;

	for_all_ca_nodes(topop, nodep)
		if ((status = sm_activate_node(topop, nodep, retry)) == VSTATUS_TIMEOUT_LIMIT)
			return status;
	for_all_switch_nodes(topop, nodep)
		if ((status = sm_activate_node(topop, nodep, retry)) == VSTATUS_TIMEOUT_LIMIT)
			return status;

	return VSTATUS_OK;
}

Status_t
sm_activate_all_switch_first(Topology_t * topop, pActivationRetry_t retry)
{
	Status_t status = VSTATUS_OK;
	Node_t * nodep;

	for_all_switch_nodes(topop, nodep)
		if ((status = sm_activate_node(topop, nodep, retry)) == VSTATUS_TIMEOUT_LIMIT)
			return status;
	for_all_ca_nodes(topop, nodep)
		if ((status = sm_activate_node(topop, nodep, retry)) == VSTATUS_TIMEOUT_LIMIT)
			return status;

	return VSTATUS_OK;
}

