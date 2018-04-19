/* A query application for OPA PA data similar to opapaquery
 * bundled in IFS distribution
 *
 */

//core API
#include <opamgt/opamgt.h>
//extensions for PA queries
#include <opamgt/opamgt_pa.h>

#include <inttypes.h>
#include <getopt.h>

void Usage(void)
{
	fprintf(stderr, "Usage:paquery [-v] [-h hfi] [-p port] -o type [-g groupName] [-l nodeLid]\n");
	fprintf(stderr, "                   [-b oob_host] [-P portNumber] [-d] [-U]\n");
	fprintf(stderr, "                   [-f focus] [-S start] [-r range] [-n imgNum]\n");
	fprintf(stderr, "                   [-O imgOff] [-y imgTime] [-m moveImgNum] [-M moveImgOff]\n");
	fprintf(stderr, "                   [-V vfName]\n");
	fprintf(stderr, "    --help             - display this help text\n");
	fprintf(stderr, "    -v/--verbose       - verbose output\n");
	fprintf(stderr, "    -h/--hfi hfi       - hfi, numbered 1..n, 0= -p port will be a system wide\n");
	fprintf(stderr, "                         port num (default is 0)\n");
	fprintf(stderr, "    -p/--port port     - port, numbered 1..n, 0=1st active (default is 1st\n");
	fprintf(stderr, "                         active)\n");
	fprintf(stderr, "    -b  oob_host       - perform out of band query. For this example, oob_host\n");
	fprintf(stderr, "                         should have format hostname[:port] or a.b.c.d[:port]\n");
	fprintf(stderr, "    -o/--output        - output type, default is groupList\n");
	fprintf(stderr, "    -g/--groupName     - group name for groupInfo query\n");
	fprintf(stderr, "    -l/--lid           - lid of node for portCounters query\n");
	fprintf(stderr, "    -P/--portNumber    - port number for portCounters query\n");
	fprintf(stderr, "    -d/--delta         - delta flag for portCounters query\n");
	fprintf(stderr, "    -U/--userCntrs     - user controlled counters flag for portCounters query\n");
	fprintf(stderr, "    -f/--focus         - focus select value for getting focus ports\n");
	fprintf(stderr, "           focus select values:\n");
	fprintf(stderr, "           utilhigh      - sorted by utilization - highest first\n");
	fprintf(stderr, "           pktrate       - sorted by packet rate - highest first\n");
	fprintf(stderr, "           utillow       - sorted by utilization - lowest first\n");
	fprintf(stderr, "           integrity     - sorted by integrity category - highest first\n");
	fprintf(stderr, "           congestion    - sorted by congestion category - highest first\n");
	fprintf(stderr, "           smacongestion - sorted by sma congestion category - highest first\n");
	fprintf(stderr, "           bubbles       - sorted by bubble category - highest first\n");
	fprintf(stderr, "           security      - sorted by security category - highest first\n");
	fprintf(stderr, "           routing       - sorted by routing category - highest first\n");
	fprintf(stderr, "    -S/--start           - start of window for focus ports - should always be 0\n");
	fprintf(stderr, "                           for now\n");
	fprintf(stderr, "    -r/--range           - size of window for focus ports list\n");
	fprintf(stderr, "    -n/--imgNum          - 64-bit image number - may be used with groupInfo,\n");
	fprintf(stderr, "                           groupConfig, portCounters (delta)\n");
	fprintf(stderr, "    -O/--imgOff          - image offset - may be used with groupInfo, groupConfig,\n");
	fprintf(stderr, "                           portCounters (delta)\n");
	fprintf(stderr, "    -y/--imgTime         - image time - may be used with imageinfo, groupInfo,\n");
	fprintf(stderr, "                           groupInfo, groupConfig, freezeImage, focusPorts,\n");
	fprintf(stderr, "                           vfInfo, vfConfig, and vfFocusPorts. Will return\n");
	fprintf(stderr, "                           closest image within image interval if possible.\n");
	fprintf(stderr, "                           Input should be seconds since 1970-01-01 00:00:00.\n");
	fprintf(stderr, "    -m/--moveImgNum      - 64-bit image number - used with moveFreeze to move a\n");
	fprintf(stderr, "                           freeze image\n");
	fprintf(stderr, "    -M/--moveImgOff      - image offset - may be used with moveFreeze to move a\n");
	fprintf(stderr, "                           freeze image\n");
	fprintf(stderr, "    -V/--vfName          - VF name for vfInfo query\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "The -h and -p options permit a variety of selections:\n");
	fprintf(stderr, "    -h 0       - 1st active port in system (this is the default)\n");
	fprintf(stderr, "    -h 0 -p 0  - 1st active port in system\n");
	fprintf(stderr, "    -h x       - 1st active port on HFI x\n");
	fprintf(stderr, "    -h x -p 0  - 1st active port on HFI x\n");
	fprintf(stderr, "    -h 0 -p y  - port y within system (irrespective of which ports are active)\n");
	fprintf(stderr, "    -h x -p y  - HFI x, port y\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Output types:\n");
	fprintf(stderr, "    classPortInfo      - class port info\n");
	fprintf(stderr, "    groupList          - list of PA groups (default)\n");
	fprintf(stderr, "    groupInfo          - summary statistics of a PA group - requires -g option\n");
	fprintf(stderr, "                         for groupName\n");
	fprintf(stderr, "    groupConfig        - configuration of a PA group - requires -g option for\n");
	fprintf(stderr, "                         groupName\n");
	fprintf(stderr, "    portCounters       - port counters of fabric port - requires -l (lid) and\n");
	fprintf(stderr, "                         -P (port) options, -d (delta) is optional\n");
	fprintf(stderr, "    pmConfig           - retrieve PM configuration information\n");
	fprintf(stderr, "    freezeImage        - create freeze frame for image ID - requires -n (imgNum)\n");
	fprintf(stderr, "    releaseImage       - release freeze frame for image ID - requires -n\n");
	fprintf(stderr, "                         (imgNum)\n");
	fprintf(stderr, "    renewImage         - renew lease for freeze frame for image ID - requires -n\n");
	fprintf(stderr, "                         (imgNum)\n");
	fprintf(stderr, "    moveFreeze         - move freeze frame from image ID to new image ID -\n");
	fprintf(stderr, "                         requires -n (imgNum) and -m (moveImgNum)\n");
	fprintf(stderr, "    focusPorts         - get sorted list of ports using utilization or error\n");
	fprintf(stderr, "                         values (from group buckets)\n");
	fprintf(stderr, "    imageInfo          - get information about a PA image (timestamps, etc.) -\n");
	fprintf(stderr, "                         requires -n (imgNum)\n");
	fprintf(stderr, "    vfList             - list of virtual fabrics\n");
	fprintf(stderr, "    vfInfo             - summary statistics of a virtual fabric - requires -V\n");
	fprintf(stderr, "                         option for vfName\n");
	fprintf(stderr, "    vfConfig           - configuration of a virtual fabric - requires -V option\n");
	fprintf(stderr, "                         for vfName\n");
	fprintf(stderr, "    vfPortCounters     - port counters of fabric port - requires -V (vfName), -l\n");
	fprintf(stderr, "                         (lid) and -P (port) options, -d (delta) is optional\n");
	fprintf(stderr, "    vfFocusPorts       - get sorted list of virtual fabric ports using\n");
	fprintf(stderr, "                         utilization or error values (from VF buckets) -\n");
	fprintf(stderr, "                         requires -V (vfname)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage examples:\n");
	fprintf(stderr, "    paquery -o classPortInfo\n");
	fprintf(stderr, "    paquery -o groupList\n");
	fprintf(stderr, "    paquery -o groupInfo -g All\n");
	fprintf(stderr, "    paquery -o groupConfig -g All\n");
	fprintf(stderr, "    paquery -o portCounters -l 1 -P 1 -d 1\n");
	fprintf(stderr, "    paquery -o portCounters -l 1 -P 1 -d 1 -n 0x20000000d02 -O 1\n");
	fprintf(stderr, "    paquery -o portCounters -l 1 -P 1 -y 1494873266 \n");
	fprintf(stderr, "    paquery -o pmConfig\n");
	fprintf(stderr, "    paquery -o freezeImage -n 0x20000000d02\n");
	fprintf(stderr, "    paquery -o releaseImage -n 0xd01\n");
	fprintf(stderr, "    paquery -o renewImage -n 0xd01\n");
	fprintf(stderr, "    paquery -o moveFreeze -n 0xd01 -m 0x20000000d02 -M -2\n");
	fprintf(stderr, "    paquery -o focusPorts -g All -f integrity -S 0 -r 20\n");
	fprintf(stderr, "    paquery -o imageInfo -n 0x20000000d02\n");
	fprintf(stderr, "    paquery -o vfList\n");
	fprintf(stderr, "    paquery -o vfInfo -V Default\n");
	fprintf(stderr, "    paquery -o vfConfig -V Default\n");
	fprintf(stderr, "    paquery -o vfPortCounters -l 1 -P 1 -d 1 -V Default\n");
	fprintf(stderr, "    paquery -o vfFocusPorts -V Default -f integrity -S 0 -r 20\n");

	exit(2);
}

typedef struct OutputFocusMap {
	char *string;
	int32 focus;
} OutputFocusMap_t;

OutputFocusMap_t OutputFocusTable[]= {
	{"utilhigh",         0x00020001},
	{"pktrate",          0x00020082},
	{"utillow",          0x00020101},
	{"integrity",        0x00030001},
	{"congestion",       0x00030002},
	{"smacongestion",    0x00030003},
	{"bubbles",          0x00030004},
	{"security" ,        0x00030005},
	{"routing",          0x00030006},
	{ NULL, 0},
};

int StringToFocus (int32 *value, const char* str)
{
	int i;

	i=0;
	while (OutputFocusTable[i].string!=NULL) {
		if (0 == strcmp(str,OutputFocusTable[i].string) ){
			*value = OutputFocusTable[i].focus;
			return 0;
		}
		else i++;
	}
	return 1;
}

// command line options
struct option options[] = {
	{ "verbose", required_argument, NULL, 'v' },
	{ "hfi", required_argument, NULL, 'h' },
	{ "port", required_argument, NULL, 'p' },
	{ "lid", required_argument, NULL, 'l' },

	{ "groupName", required_argument, NULL, 'g' },
	{ "portNumber", required_argument, NULL, 'P' },
	{ "delta", no_argument, NULL, 'd' },
	{ "userCntrs", no_argument, NULL, 'U' },
	{ "focus", required_argument, NULL, 'f' },
	{ "start", required_argument, NULL, 'S' },
	{ "range", required_argument, NULL, 'r' },
	{ "imgNum", required_argument, NULL, 'n' },
	{ "imgOff", required_argument, NULL, 'O' },
	{ "imgTime", required_argument, NULL, 'y' },
	{ "moveImgNum", required_argument, NULL, 'm' },
	{ "moveImgOff", required_argument, NULL, 'M' },
	{ "vfName", required_argument, NULL, 'V' },

	// output type
	{ "output", required_argument, NULL, 'o' },
	{ "help", no_argument, NULL, '$' },	// use an invalid option character
	{ 0 }
};

int main(int argc, char **argv)
{
	OMGT_STATUS_T status = OMGT_STATUS_SUCCESS;
	int exitcode = 0;
	struct omgt_port * port = NULL;
	int i,c,index;
	uint32_t num_data;

	int debug = 0;

	char * type = "groupList";
	int hfi_num = 1;
	int port_num = 1;
	char * oob_addr = NULL;

	int select,start,range;
	char name_data[256];

	int lid = 1, counters_port = 1;

	int delta, user_counters;

	//where applicable, get current data by default
	STL_PA_IMAGE_ID_DATA image_id_query = {.imageNumber = PACLIENT_IMAGE_CURRENT};
	STL_PA_IMAGE_ID_DATA response_id;

	STL_PA_IMAGE_ID_DATA image_id_move = {0};
	while (-1 != (c = getopt_long(argc, argv, "h:p:o:l:P:n:g:dUO:y:m:M:f:S:r:V:b:v$", options, &index))){
		switch (c)
		{
			case '$':
				Usage();
				break;
			case 'h':
				hfi_num = strtoul(optarg, NULL, 0);
				break;
			case 'p':
				port_num = strtoul(optarg, NULL, 0);
				break;
			case 'o':
				type = optarg;
				break;
			case 'l':
				lid = strtoul(optarg, NULL, 0);
				break;
			case 'P':
				counters_port = strtoul(optarg, NULL, 0);
				break;
			case 'n':
				image_id_query.imageNumber = strtoul(optarg, NULL, 0);
			case 'g':
				snprintf(name_data, 256, "%s", optarg);
				break;
			case 'd':
				delta = 1;
				break;
			case 'U':
				user_counters = 1;
				break;
			case 'O':
				image_id_query.imageOffset = strtol(optarg, NULL, 0);
				break;
			case 'y':
				image_id_query.imageNumber = PACLIENT_IMAGE_TIMED;
				image_id_query.imageTime.absoluteTime = strtoul(optarg, NULL, 0);
				break;
			case 'm':
				image_id_move.imageNumber = strtoul(optarg, NULL, 0);
				break;
			case 'M':
				image_id_move.imageOffset = strtol(optarg, NULL, 0);
				break;
			case 'f':
				if (StringToFocus (&select, optarg)) {
					fprintf(stderr, "opapaquery: Invalid Focus: %s\n", optarg);
					Usage();
				}
				break;
			case 'S':
				start = strtoul(optarg, NULL, 0);
				break;
			case 'r':
				range = strtoul(optarg, NULL, 0);
				break;
			case 'V':
				snprintf(name_data, 256, "%s", optarg);
				break;
			case 'v':
				debug = 1;
				break;
			case 'b':
				oob_addr = optarg;
				break;
			default:
				fprintf(stderr, "opasaquery: Invalid option -%c\n", c);
				Usage();
				break;
		}
	}

	// create a session
	struct omgt_params session_params = {
		.debug_file = debug ? stderr : NULL,
		.error_file = stderr
	};
	if (oob_addr){
		char *tmp = strchr(oob_addr, ':');
		if (tmp) *tmp = 0;
		struct omgt_oob_input oob_input = {
			/* host can be specified by name or as either IPv4 of IPv6
			 * but this example only considers IPv4 for simplicity*/
			.host = oob_addr,
			.port = (tmp ? strtol(++tmp, NULL, 0) : 3245)
		};

		status = omgt_oob_connect(&port, &oob_input, &session_params);
	} else {
		status = omgt_open_port_by_num(&port, hfi_num, port_num, &session_params);
	}

	if (OMGT_STATUS_SUCCESS != status){
		fprintf(stderr, "failed to open port\n");
		exitcode=1;
		goto fail1;
	}

	void * pa_data;
	//perform the requested operation
	if (!strcasecmp(type, "pmconfig")){
		STL_PA_PM_CFG_DATA pa_data;

		status = omgt_pa_get_pm_config(port, &pa_data);
		if (OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to execute query. MadStatus=0x%x\n", omgt_get_pa_mad_status(port));
		} else {
			printf("Sweep interval: %d seconds\n", pa_data.sweepInterval);
		}
	} else if (!strcasecmp(type, "imageInfo")){
		STL_PA_IMAGE_INFO_DATA pa_data;

		status = omgt_pa_get_image_info(port, image_id_query, &pa_data);
		if (OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to execute query. MadStatus=0x%x\n", omgt_get_pa_mad_status(port));
		} else {
			printf("Image ID: 0x%"PRIx64"\n", pa_data.imageId.imageNumber);
		}
	} else if (!strcasecmp(type, "groupList")){
		STL_PA_GROUP_LIST *pa_data = NULL;

		status = omgt_pa_get_group_list(port, &num_data, &pa_data);
		if (OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to execute query. MadStatus=0x%x\n", omgt_get_pa_mad_status(port));
		} else {
			for (i = 0; i < num_data; ++i){
				printf("Group Name: %s\n", pa_data[i].groupName);
			}
			// Some calls allocate memory and have associated release functions to free that memory
			omgt_pa_release_group_list(&pa_data);
		}
	} else if (!strcasecmp(type, "vfList")){
		STL_PA_VF_LIST *pa_data = NULL;

		status = omgt_pa_get_vf_list(port, &num_data, &pa_data);
		if (OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to execute query. MadStatus=0x%x\n", omgt_get_pa_mad_status(port));
		} else {
			for(i = 0; i < num_data; ++i){
				printf("VF: %s\n", pa_data[i].vfName);
			}
			omgt_pa_release_vf_list(&pa_data);
		}
	} else if (!strcasecmp(type, "groupInfo")){
		STL_PA_PM_GROUP_INFO_DATA pa_data;

		status = omgt_pa_get_group_info(port, image_id_query, name_data, &response_id, &pa_data);
		if (OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to execute query. MadStatus=0x%x\n", omgt_get_pa_mad_status(port));
		} else {
			printf("Group internal ports: %d\n", pa_data.numInternalPorts);
		}
	} else if (!strcasecmp(type, "vfInfo")){
		STL_PA_VF_INFO_DATA pa_data;

		status = omgt_pa_get_vf_info(port, image_id_query, name_data, &response_id, &pa_data);
		if (OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to execute query. MadStatus=0x%x\n", omgt_get_pa_mad_status(port));
		} else {
			printf("VF ports: %d\n", pa_data.numPorts);
		}
	} else if (!strcasecmp(type, "groupConfig")){
		STL_PA_PM_GROUP_CFG_RSP * pa_data = NULL;

		status = omgt_pa_get_group_config(port, image_id_query, name_data, &response_id, &num_data, &pa_data);
		if (OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to execute query. MadStatus=0x%x\n", omgt_get_pa_mad_status(port));
		} else {
			for(i = 0; i < num_data; ++i){
				printf("Group Member: %s\n", pa_data[i].nodeDesc);
			}
			omgt_pa_release_group_config(&pa_data);
		}
	} else if (!strcasecmp(type, "vfConfig")){
		STL_PA_VF_CFG_RSP *pa_data = NULL;

		status = omgt_pa_get_vf_config(port, image_id_query, name_data, &response_id, &num_data, &pa_data);
		if (OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to execute query. MadStatus=0x%x\n", omgt_get_pa_mad_status(port));
		} else {
			for(i = 0; i < num_data; ++i){
				printf("VF Member: %s\n", pa_data[i].nodeDesc);
			}
			omgt_pa_release_vf_config(&pa_data);
		}
	} else if (!strcasecmp(type, "focusPorts")){
		STL_FOCUS_PORTS_RSP *pa_data = NULL;

		status = omgt_pa_get_group_focus(port, image_id_query, name_data, select, start, range, &response_id, &num_data, &pa_data);
		if (OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to execute query. MadStatus=0x%x\n", omgt_get_pa_mad_status(port));
		} else {
			for (i = 0; i < num_data; ++i){
				printf("Port: %s  Value %"PRIu64"\n", pa_data[i].nodeDesc, pa_data[i].value);
			}
			omgt_pa_release_group_focus(&pa_data);
		}
	} else if (!strcasecmp(type, "vfFocusPorts")){
		STL_PA_VF_FOCUS_PORTS_RSP *pa_data = NULL;

		status = omgt_pa_get_vf_focus(port, image_id_query, name_data, select, start, range, &response_id, &num_data, &pa_data);
		if (OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to execute query. MadStatus=0x%x\n", omgt_get_pa_mad_status(port));
		} else {
			for(i = 0; i < num_data; ++i){
				printf("Port: %s Value %"PRIu64"\n", pa_data[i].nodeDesc, pa_data[i].value);
			}
			omgt_pa_release_vf_focus(&pa_data);
		}
	} else if (!strcasecmp(type, "portCounters")){
		STL_PORT_COUNTERS_DATA pa_data;

		status = omgt_pa_get_port_stats(port, image_id_query, lid, counters_port, &response_id, &pa_data, NULL, delta, user_counters);
		if (OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to execute query. MadStatus=0x%x\n", omgt_get_pa_mad_status(port));
		} else {
			printf("Port Xmit Data: %"PRIu64"\n", pa_data.portXmitData);
		}
	} else if (!strcasecmp(type, "vfPortCounters")){
		STL_PA_VF_PORT_COUNTERS_DATA pa_data;

		status = omgt_pa_get_vf_port_stats(port, image_id_query, name_data, lid, counters_port, &response_id, &pa_data, NULL, delta, user_counters);
		if (OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to execute query. MadStatus=0x%x\n", omgt_get_pa_mad_status(port));
		} else {
			printf("Port Xmit Data: %"PRIu64"\n", pa_data.portVFXmitData);
		}
	} else if (!strcasecmp(type, "freezeImage")){
		STL_PA_IMAGE_ID_DATA pa_data;

		status = omgt_pa_freeze_image(port, image_id_query, &pa_data);
		if (OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to execute query. MadStatus=0x%x\n", omgt_get_pa_mad_status(port));
		} else {
			printf("Successfully froze image 0x%"PRIx64"\n", pa_data.imageNumber);
		}
	} else if (!strcasecmp(type, "moveFreeze")){
		status = omgt_pa_move_image_freeze(port, image_id_query, &image_id_move);
		if (OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to execute query. MadStatus=0x%x\n", omgt_get_pa_mad_status(port));
		} else {
			printf("Success\n");
		}

	} else if (!strcasecmp(type, "releaseImage")){
		status = omgt_pa_release_image(port, image_id_query);
		if (OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to execute query. MadStatus=0x%x\n", omgt_get_pa_mad_status(port));
		} else {
			printf("Success\n");
		}
	} else if (!strcasecmp(type, "renewImage")){
		status = omgt_pa_renew_image(port, image_id_query);
		if (OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to execute query. MadStatus=0x%x\n", omgt_get_pa_mad_status(port));
		} else {
			printf("Success\n");
		}
	} else if (!strcasecmp(type, "classPortInfo")){
		STL_CLASS_PORT_INFO * pa_data;
		status = omgt_pa_get_classportinfo(port, &pa_data);

		if(OMGT_STATUS_SUCCESS != status){
			fprintf(stderr, "failed to get ClassPortInfo\n");
		} else {
			printf("PA Capability Mask: 0x%x\n", pa_data->CapMask);

			//must free when done
			free(pa_data);
		}
	}

fail2:
	omgt_close_port(port);
fail1:
	return exitcode;
}
