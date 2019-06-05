/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * .. _pubsub-tutorial:
 *
 * Working with Publish/Subscribe
 * ------------------------------
 *
 * Work in progress: This Tutorial will be continuously extended during the next
 * PubSub batches. More details about the PubSub extension and corresponding
 * open62541 API are located here: :ref:`pubsub`.
 *
 * Publishing Fields
 * ^^^^^^^^^^^^^^^^^
 * The PubSub publish example demonstrate the simplest way to publish
 * informations from the information model over UDP multicast using the UADP
 * encoding.
 *
 * **Connection handling**
 *
 * PubSubConnections can be created and deleted on runtime. More details about
 * the system preconfiguration and connection can be found in
 * ``tutorial_pubsub_connection.c``.
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_ethernet.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/server_config.h>

#include <signal.h>
#include <time.h>
#include <stdlib.h>


UA_NodeId connectionIdent, publishedDataSetIdent, writerGroupIdent;
UA_Variant           valueVariant;
UA_Boolean           actVal;

int counter = 0;
int sample_count = 10;
int publish_interval = 1000;
int write_rate = 1000;
UA_Boolean running = true;
UA_Boolean samples = false;

static void
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl){
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric = UA_UInt32_random();
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
}

/**
 * **PublishedDataSet handling**
 *
 * The PublishedDataSet (PDS) and PubSubConnection are the toplevel entities and
 * can exist alone. The PDS contains the collection of the published fields. All
 * other PubSub elements are directly or indirectly linked with the PDS or
 * connection. */
static void
addPublishedDataSet(UA_Server *server) {
    /* The PublishedDataSetConfig contains all necessary public
    * informations for the creation of a new PublishedDataSet */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    /* Create new PublishedDataSet based on the PublishedDataSetConfig. */
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);
}

static void updateCurrentTime(UA_Server *server, void * data) {
    UA_NodeId timeNodeId = UA_NODEID_NUMERIC(1, 52510);

    struct timespec tp1;
    clock_gettime(CLOCK_REALTIME, &tp1);
    UA_UInt64 tp1_total = (UA_UInt64)(tp1.tv_sec*1000000000 + tp1.tv_nsec);

    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &tp1_total, &UA_TYPES[UA_TYPES_UINT64]);
    UA_Server_writeValue(server, timeNodeId, value);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "[Time]: %lu", tp1_total);

    if (samples){
        counter++;

        if (counter == sample_count){
            running = false;
        }
    }

}

UA_UInt32 sequence_count = 0;

/*static void updateCounter(UA_Server *server, void * data) {

    sequence_count++;

    UA_NodeId counterNodeId = UA_NODEID_NUMERIC(1, 52525);
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &sequence_count, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Server_writeValue(server, counterNodeId, value);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "[Sequence]: %u", sequence_count);

    if (samples){
        counter++;

        if (counter == sample_count){
            running = false;
        }
    }

}*/

/*
static void
addIntegerVariable(UA_Server *server, UA_UInt16 nsIndex, UA_UInt32 numIdent, UA_UInt32 integer) {
    */
/* Define the attribute of the myInteger variable node *//*

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_UInt32 myInteger = integer;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    */
/* Add the variable node to the information model *//*

    UA_NodeId myIntegerNodeId = UA_NODEID_NUMERIC(nsIndex, numIdent);
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}
*/


static void
addTimeVariable(UA_Server *server, UA_UInt16 nsIndex, UA_UInt32 numIdent) {

    UA_Int64 tp1_total = 0;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &tp1_total, &UA_TYPES[UA_TYPES_UINT64]);

    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","Clock real time");
    attr.dataType = UA_TYPES[UA_TYPES_UINT64].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId timeNodeId = UA_NODEID_NUMERIC(nsIndex, numIdent);
    UA_QualifiedName timeName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, timeNodeId, parentNodeId,
                              parentReferenceNodeId, timeName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);

}

/**
 * **DataSetField handling**
 *
 * The DataSetField (DSF) is part of the PDS and describes exactly one published
 * field. */

static void
addNewDataSetField(UA_Server *server, UA_UInt16 nsIndex, UA_UInt16 numIdent, char* fieldNameAlias){
    /* Add a field to the previous created PublishedDataSet */
    UA_NodeId dataSetFieldIdent;
    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING(fieldNameAlias);
    dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable =
            UA_NODEID_NUMERIC(nsIndex, numIdent);
    dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_Server_addDataSetField(server, publishedDataSetIdent,
                              &dataSetFieldConfig, &dataSetFieldIdent);
}

/**
 * **WriterGroup handling**
 *
 * The WriterGroup (WG) is part of the connection and contains the primary
 * configuration parameters for the message creation. */
static void
addWriterGroup(UA_Server *server) {
    /* Now we create a new WriterGroupConfig and add the group to the existing
     * PubSubConnection. */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = publish_interval;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    /* The configuration flags for the messages are encapsulated inside the
     * message- and transport settings extension objects. These extension
     * objects are defined by the standard. e.g.
     * UadpWriterGroupMessageDataType */
    UA_Server_addWriterGroup(server, connectionIdent, &writerGroupConfig, &writerGroupIdent);
}

/**
 * **DataSetWriter handling**
 *
 * A DataSetWriter (DSW) is the glue between the WG and the PDS. The DSW is
 * linked to exactly one PDS and contains additional informations for the
 * message generation. */
static void
addDataSetWriter(UA_Server *server) {
    /* We need now a DataSetWriter within the WriterGroup. This means we must
     * create a new DataSetWriterConfig and add call the addWriterGroup function. */
    UA_NodeId dataSetWriterIdent;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    dataSetWriterConfig.keyFrameCount = 10;
    UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent,
                               &dataSetWriterConfig, &dataSetWriterIdent);
}

/**
 * That's it! You're now publishing the selected fields. Open a packet
 * inspection tool of trust e.g. wireshark and take a look on the outgoing
 * packages. The following graphic figures out the packages created by this
 * tutorial.
 *
 * .. figure:: ua-wireshark-pubsub.png
 *     :figwidth: 100 %
 *     :alt: OPC UA PubSub communication in wireshark
 *
 * The open62541 subscriber API will be released later. If you want to process
 * the the datagrams, take a look on the ua_network_pubsub_networkmessage.c
 * which already contains the decoding code for UADP messages.
 *
 * It follows the main server code, making use of the above definitions. */

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static int run(UA_String *transportProfile,
               UA_NetworkAddressUrlDataType *networkAddressUrl) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    /* Details about the connection configuration and handling are located in
     * the pubsub connection tutorial */
    config->pubsubTransportLayers =
        (UA_PubSubTransportLayer *) UA_calloc(2, sizeof(UA_PubSubTransportLayer));
    if(!config->pubsubTransportLayers) {
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    config->pubsubTransportLayers[1] = UA_PubSubTransportLayerEthernet();
    config->pubsubTransportLayersSize++;
#endif

    addPubSubConnection(server, transportProfile, networkAddressUrl);
    addPublishedDataSet(server);

    addTimeVariable(server, 1, 52510);

    addNewDataSetField(server, 1, 52510, "TimeInteger");

    addWriterGroup(server);

    addDataSetWriter(server);

    UA_Server_addRepeatedCallback(server, updateCurrentTime, NULL, write_rate, NULL);
    // UA_Server_addRepeatedCallback(server, updateCounter, NULL, write_rate, NULL);

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void
usage(char *progname) {
    printf("usage: %s <uri> [device]\n", progname);
}

int main(int argc, char **argv) {
    UA_String transportProfile =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        } else if (strncmp(argv[1], "opc.udp://", 10) == 0) {
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else if (strncmp(argv[1], "opc.eth://", 10) == 0) {
            transportProfile =
                UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
            if (argc < 3) {
                printf("Error: UADP/ETH needs an interface name\n");
                return EXIT_FAILURE;
            }
            networkAddressUrl.networkInterface = UA_STRING(argv[2]);
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else if (strcmp(argv[1], "-sample") == 0){
                if (argc < 3){
                    printf("Error: Number of samples not supplied\n");
                    return EXIT_FAILURE;
                }

                sample_count = atoi(argv[2]);
                samples = true;
                printf("samples = %d\n", sample_count);

                if (argc > 3 && strcmp(argv[3], "-interval") == 0){
                    if (argc < 5){
                        printf("Error: Interval not supplied\n");
                        return EXIT_FAILURE;
                    }

                    publish_interval = atoi(argv[4]);

                    if (publish_interval < 50){
                        printf("Warning: Interval cannot be less than 50 milliseconds\n");
                        printf("Setting interval to 50 milliseconds\n");
                        publish_interval = 50;
                    }
                    printf("interval = %d\n", publish_interval);

                    if (argc > 5 && strcmp(argv[5], "-write_rate") == 0)
                        if (argc < 7){
                        printf("Warning: Updation Rate not supplied. Setting write rate equal to publish_interval\n");
                        write_rate = publish_interval;
                    }

                    write_rate = atoi(argv[6]);
                }

            }
        else {
            printf("Error: unknown URI\n");
            return EXIT_FAILURE;
        }
    }

    return run(&transportProfile, &networkAddressUrl);
}
