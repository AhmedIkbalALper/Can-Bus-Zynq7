#include "xcanps.h"
#include "xparameters.h"
#include "xil_printf.h"

#define CAN0_DEVICE_ID    XPAR_XCANPS_0_DEVICE_ID
#define CAN1_DEVICE_ID    XPAR_XCANPS_1_DEVICE_ID

#define TEST_MESSAGE_ID   2000
#define FRAME_DATA_LENGTH 8
#define TEST_MESSAGE_ID   2000
#define FRAME_DATA_LENGTH 8
#define XCANPS_MAX_FRAME_SIZE_IN_WORDS (XCANPS_MAX_FRAME_SIZE / sizeof(u32))
#define ASCII_TEXT "AKIF"
#define ASCII_TEXT_LENGTH (sizeof(ASCII_TEXT) - 1)


int CanPsSetup(XCanPs *InstancePtr, u16 DeviceId);
int SendText(XCanPs *InstancePtr, const char *Text, int Length);
int RecvText(XCanPs *InstancePtr, char *Buffer, int MaxLength);

XCanPs Can0;

int CanPsSetup(XCanPs *InstancePtr, u16 DeviceId);
int SendText(XCanPs *InstancePtr, const char *Text, int Length);
int RecvText(XCanPs *InstancePtr, char *Buffer, int MaxLength);

int main(void)
{
    int Status;
    char ReceivedText[ASCII_TEXT_LENGTH + 1] = {0};

    xil_printf("CAN0 Normal Mode ASCII Text Test\r\n");

    Status = CanPsSetup(&Can0, CAN0_DEVICE_ID);
    if (Status != XST_SUCCESS) {
        xil_printf("CAN0 setup failed\r\n");
        return XST_FAILURE;
    }

    // Send ASCII text from CAN0
    Status = SendText(&Can0, ASCII_TEXT, ASCII_TEXT_LENGTH);
    if (Status != XST_SUCCESS) {
        xil_printf("Failed to send text from CAN0\r\n");
        return XST_FAILURE;
    }

    // Receive ASCII text on CAN0
    Status = RecvText(&Can0, ReceivedText, ASCII_TEXT_LENGTH);
    if (Status != XST_SUCCESS) {
        xil_printf("Failed to receive text on CAN0\r\n");
        return XST_FAILURE;
    }

    // Null-terminate the received text
    ReceivedText[ASCII_TEXT_LENGTH] = '\0';

    xil_printf("Received text: %s\r\n", ReceivedText);
    return XST_SUCCESS;
}

int CanPsSetup(XCanPs *InstancePtr, u16 DeviceId)
{
    XCanPs_Config *ConfigPtr;
    int Status;

    ConfigPtr = XCanPs_LookupConfig(DeviceId);
    if (ConfigPtr == NULL) {
        return XST_FAILURE;
    }

    Status = XCanPs_CfgInitialize(InstancePtr, ConfigPtr, ConfigPtr->BaseAddr);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    Status = XCanPs_SelfTest(InstancePtr);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    XCanPs_EnterMode(InstancePtr, XCANPS_MODE_CONFIG);
    while (XCanPs_GetMode(InstancePtr) != XCANPS_MODE_CONFIG);

    XCanPs_SetBaudRatePrescaler(InstancePtr, 29);
    XCanPs_SetBitTiming(InstancePtr, 3, 2, 15);

    // Enter Normal Mode for physical loopback test
    XCanPs_EnterMode(InstancePtr, XCANPS_MODE_NORMAL);
    while (XCanPs_GetMode(InstancePtr) != XCANPS_MODE_NORMAL);

    return XST_SUCCESS;
}

int SendText(XCanPs *InstancePtr, const char *Text, int Length)
{
    u32 TxFrame[XCANPS_MAX_FRAME_SIZE_IN_WORDS];
    int Index = 0;
    int Status;

    while (Index < Length) {
        // Create CAN frame
        TxFrame[0] = XCanPs_CreateIdValue(TEST_MESSAGE_ID, 0, 0, 0, 0);
        TxFrame[1] = XCanPs_CreateDlcValue(FRAME_DATA_LENGTH);

        // Fill CAN frame with text data
        for (int i = 0; i < FRAME_DATA_LENGTH; i++) {
            ((u8 *)&TxFrame[2])[i] = (Index < Length) ? Text[Index++] : 0;
        }

        // Wait until TX FIFO has room
        while (XCanPs_IsTxFifoFull(InstancePtr) == TRUE);

        // Send the frame
        Status = XCanPs_Send(InstancePtr, TxFrame);
        if (Status != XST_SUCCESS) {
            return Status;
        }
    }

    return XST_SUCCESS;
}

int RecvText(XCanPs *InstancePtr, char *Buffer, int MaxLength)
{
    u32 RxFrame[XCANPS_MAX_FRAME_SIZE_IN_WORDS];
    int Index = 0;
    int Status;

    while (Index < MaxLength) {
        // Wait until a frame is received
        while (XCanPs_IsRxEmpty(InstancePtr) == TRUE);

        // Receive the frame
        Status = XCanPs_Recv(InstancePtr, RxFrame);
        if (Status != XST_SUCCESS) {
            return Status;
        }

        // Extract text data from CAN frame
        for (int i = 0; i < FRAME_DATA_LENGTH; i++) {
            if (Index < MaxLength) {
                Buffer[Index++] = ((u8 *)&RxFrame[2])[i];
            }
        }
    }

    return XST_SUCCESS;
}
