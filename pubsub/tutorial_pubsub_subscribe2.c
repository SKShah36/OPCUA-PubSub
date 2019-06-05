/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

/**
 * IMPORTANT ANNOUNCEMENT
 * The PubSub subscriber API is currently not finished. This examples can be used to
 * receive and print the values, which are published by the tutorial_pubsub_publish
 * example. The following code uses internal API which will be later replaced by the
 * higher-level PubSub subscriber API. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types_generated.h>

#include "ua_pubsub.h"
#include "ua_pubsub_networkmessage.h"
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
#include <open62541/plugin/pubsub_ethernet.h>
#endif

#include <time.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

size_t counter = 0;
size_t sample_count = 10;
int poll_count1 = 0;

typedef struct measurements{
    int sequence;
    UA_UInt64 receiveTime;
    UA_UInt64 currentTime;
}measurement;

measurement *measure;

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

//Allocate memory for measurement structure


static void
subscriptionPollingCallback(UA_Server *server, UA_PubSubConnection *connection) {

    UA_ByteString buffer;
    if (UA_ByteString_allocBuffer(&buffer, 512) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Message buffer allocation failed!");
        return;
    }

    /* Receive the message. Blocks for 5ms */
    UA_StatusCode retval =
        connection->channel->receive(connection->channel, &buffer, NULL, 5);
    if(retval != UA_STATUSCODE_GOOD || buffer.length == 0) {
        /* Workaround!! Reset buffer length. Receive can set the length to zero.
         * Then the buffer is not deleted because no memory allocation is
         * assumed.
         * TODO: Return an error code in 'receive' instead of setting the buf
         * length to zero. */
        buffer.length = 512;
        UA_ByteString_clear(&buffer);
        return;
    }

    /* Decode the message */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Message length: %lu", (unsigned long) buffer.length);
    UA_NetworkMessage networkMessage;
    memset(&networkMessage, 0, sizeof(UA_NetworkMessage));
    size_t currentPosition = 0;
    UA_NetworkMessage_decodeBinary(&buffer, &currentPosition, &networkMessage);
    UA_ByteString_clear(&buffer);

    /* Is this the correct message type? */
    if(networkMessage.networkMessageType != UA_NETWORKMESSAGE_DATASET)
        goto cleanup;

    /* At least one DataSetMessage in the NetworkMessage? */
    if(networkMessage.payloadHeaderEnabled &&
       networkMessage.payloadHeader.dataSetPayloadHeader.count < 1)
        goto cleanup;

    /* Is this a KeyFrame-DataSetMessage? */
    UA_DataSetMessage *dsm = &networkMessage.payload.dataSetPayload.dataSetMessages[0];
    if(dsm->header.dataSetMessageType != UA_DATASETMESSAGE_DATAKEYFRAME)
        goto cleanup;



    /* Loop over the fields and print well-known content types */
    for(int i = 0; i < dsm->data.keyFrameData.fieldCount; i++) {

        const UA_DataType *currentType = dsm->data.keyFrameData.dataSetFields[i].value.type;

        if (currentType == &UA_TYPES[UA_TYPES_UINT64]) {

            UA_UInt64 receivedTime = *(UA_UInt64 *)dsm->data.keyFrameData.dataSetFields[i].value.data;
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Message content: [Time] \tReceived time integer: %lu", receivedTime);


            struct timespec tp1;
            clock_gettime(CLOCK_REALTIME, &tp1);
            UA_UInt64 tp1_total = (UA_UInt64)(tp1.tv_sec*1000000000 + tp1.tv_nsec);

            measure[poll_count1].sequence = poll_count1 + 1;
            measure[poll_count1].currentTime = tp1_total;
            measure[poll_count1].receiveTime = receivedTime;

            printf("%d,%lu,%lu\n", measure[poll_count1].sequence, measure[poll_count1].receiveTime, measure[poll_count1].currentTime);
            poll_count1++;
            counter++;

            if (counter == sample_count){

                FILE *f = fopen("perf_log.csv", "w+");

                for (int j = 0; j < 10; j++){
                    printf("%d,%lu,%lu\n", measure[j].sequence, measure[j].receiveTime, measure[j].currentTime);
                    fprintf(f, "%d,%lu,%lu\n", measure[j].sequence, measure[j].receiveTime, measure[j].currentTime);
                }

                fclose(f);
                free(measure);
                running = false;
            }
        }
    }

 cleanup:
    UA_NetworkMessage_clear(&networkMessage);
}

static int
run(UA_String *transportProfile, UA_NetworkAddressUrlDataType *networkAddressUrl) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, 4801, NULL);

    measure = (measurement *)malloc(sample_count*sizeof(measurement));

    /* Details about the PubSubTransportLayer can be found inside the
     * tutorial_pubsub_connection */
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *)
        UA_calloc(2, sizeof(UA_PubSubTransportLayer));
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

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    UA_NodeId connectionIdent;
    UA_StatusCode retval =
        UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
    if(retval == UA_STATUSCODE_GOOD)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                    "The PubSub Connection was created successfully!");

    /* The following lines register the listening on the configured multicast
     * address and configure a repeated job, which is used to handle received
     * messages. */
    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, connectionIdent);
    if(connection != NULL) {
        UA_StatusCode rv = connection->channel->regist(connection->channel, NULL, NULL);
        if (rv == UA_STATUSCODE_GOOD) {
            UA_UInt64 subscriptionCallbackId;
            UA_Server_addRepeatedCallback(server, (UA_ServerCallback)subscriptionPollingCallback,
                                          connection, 100, &subscriptionCallbackId);
        } else {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "register channel failed: %s!",
                           UA_StatusCode_name(rv));
        }
    }

    retval |= UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;;
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
        }
        else if (strcmp(argv[1], "-sample") == 0){
                if (argc < 3){
                    printf("Error: Number of samples not supplied\n");
                    return EXIT_FAILURE;
                }
                char *ptr;
                sample_count = strtoul(argv[2], &ptr, 10);
                printf("samples = %lu\n", sample_count);

            }
        else {
            printf("Error: unknown URI\n");
            return EXIT_FAILURE;
        }
    }

    return run(&transportProfile, &networkAddressUrl);
}
