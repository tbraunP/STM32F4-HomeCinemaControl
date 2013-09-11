#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

#include "tasks/Task_Priorities.h"
#include "tasks/tcpecho.h"

#include "lwip/opt.h"


#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"


/*-----------------------------------------------------------------------------------*/
void tcpecho_thread(void *arg) {
    struct netconn *conn, *newconn;
    err_t err;

    LWIP_UNUSED_ARG(arg);

    /* Create a new connection identifier. */
    conn = netconn_new(NETCONN_TCP);

    if (conn!=NULL)
    {
        /* Bind connection to well known port number 7. */
        err = netconn_bind(conn, NULL, 7);

        if (err == ERR_OK)
        {
            /* Tell connection to go into listening mode. */
            netconn_listen(conn);

            while (1)
            {
                /* Grab new connection. */
                err_t xErr = netconn_accept(conn, &newconn);

                /* Process the new connection. */
                if (xErr== ERR_OK)
                {
                    struct netbuf *buf;
                    void *data;
                    u16_t len;

                    while ((xErr = netconn_recv(newconn, &buf)) == ERR_OK)
                    {
                        do
                        {
                            netbuf_data(buf, &data, &len);
                            netconn_write(newconn, data, len, NETCONN_COPY);

                        }
                        while (netbuf_next(buf) >= 0);

                        netbuf_delete(buf);
                    }

                    /* Close connection and discard connection identifier. */
                    netconn_close(newconn);
                    netconn_delete(newconn);
                }
            }
        }
        else
        {
            printf(" can not bind TCP netconn");
        }
    }
    else
    {
        printf("can not create TCP netconn");
    }
}
/*-----------------------------------------------------------------------------------*/

void tcpecho_init(void) {
    xTaskCreate(tcpecho_thread, (const signed char * const ) "TCP_ECHO",
                configMINIMAL_STACK_SIZE, NULL, TCPECHO_TASK_PRIO, NULL);
    //sys_thread_new("tcpecho_thread", tcpecho_thread, NULL, DEFAULT_THREAD_STACKSIZE, TCPECHO_THREAD_PRIO);
}
/*-----------------------------------------------------------------------------------*/

#endif /* LWIP_NETCONN */
