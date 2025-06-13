#include "ringBuffer.h"

/****************************** Buffer Initialization *******************************
 * @brief  Initializes the ring buffer by resetting head and tail indices.
 *
 * @param  buff: Pointer to ring buffer structure
 */
void ringBuffer_init(ringBuffer_Typedef* buff)
{
    buff->head = 0;   // Set head index to 0
    buff->tail = 0;   // Set tail index to 0
}

/********************************** Buffer Status Flags ******************************
 * @brief  Checks if the ring buffer is full.
 *
 * @param  buff: Pointer to ring buffer structure
 * @return 1 if buffer is full, 0 otherwise
 */
uint8_t ringBuffer_isFull(ringBuffer_Typedef* buff)
{
    uint8_t nextHead = (buff->head + 1) % MAX_SIZE_RING_BUFFER; // Calculate next head index
    return (nextHead == buff->tail);                            // Buffer is full if next head catches up with tail
}

/**
 * @brief  Checks if the ring buffer is empty.
 *
 * @param  buff: Pointer to ring buffer structure
 * @return 1 if buffer is empty, 0 otherwise
 */
uint8_t ringBuffer_isEmpty(ringBuffer_Typedef* buff)
{
    return (buff->head == buff->tail); // Buffer is empty if head equals tail
}

/****************************** Buffer Read and Write Operations *********************
 * @brief  Writes a byte to the ring buffer if not full.
 *
 * @param  buff: Pointer to ring buffer structure
 * @param  data: Byte to write
 */
void ringBuffer_Write(ringBuffer_Typedef* buff, uint8_t data)
{
    if (!(ringBuffer_isFull(buff))) // Only write if buffer is not full
    {
        buff->buffer[buff->head] = data;                        // Store data at head position
        buff->head = ((buff->head + 1) % MAX_SIZE_RING_BUFFER); // Advance head with wrap-around
    }
}

/**
 * @brief  Reads a byte from the ring buffer if not empty.
 *
 * @param  buff: Pointer to ring buffer structure
 * @return Byte read from buffer, or 0xFF if buffer is empty
 */
uint8_t ringBuffer_Read(ringBuffer_Typedef* buff)
{
    if (!(ringBuffer_isEmpty(buff))) // Only read if buffer is not empty
    {
        uint8_t data = buff->buffer[buff->tail];                // Read data at tail position
        buff->tail = ((buff->tail + 1) % MAX_SIZE_RING_BUFFER); // Advance tail with wrap-around
        return data;
    }
    
    return 0xFF; // Return 0xFF if buffer is empty (error/sentinel value)
}