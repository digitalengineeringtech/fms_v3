bool fms_uart2_begin(bool flag, int baudrate) {
  if (flag) {
    fms_uart2_serial.begin(baudrate, SERIAL_8N1, RXD2, TXD2);  // RXD2 and TXD2 are the GPIO pins for RX and TX
    if (fms_uart2_serial) {
      vTaskDelay(pdMS_TO_TICKS(1000));  // Wait for 1 second before repeating
      return true;
    } else {
      return false;
    }
  }
}

void fm_rx_irq_interrupt() {  // interrupt RS485/RS232 function
  uint8_t Buffer[30];
  int bytes_received = 0;
  uint16_t size = fms_uart2_serial.available();  // serial.available
  FMS_LOG_DEBUG("Got bytes on serial : %d\n", size);
  while (fms_uart2_serial.available() && bytes_received < sizeof(Buffer)) {
    yield();
    Buffer[bytes_received] = fms_uart2_serial.read();
    bytes_received++;
  }
  if(bytes_received > 0) {
    FMS_LOG_DEBUG("\n uart2 data process \n\r");
    FMS_LOG_DEBUG("uart2 data : %s\n\r", Buffer);
    FMS_LOG_DEBUG("uart2 data length : %d\n\r", bytes_received);
    UART_RECEIVE_STATE = true;
    fms_uart2_decode(Buffer, bytes_received);  // decode uart2 data main function
  }
 
}

void fms_uart2_decode(uint8_t* data, uint32_t len) {
  
  // Print the raw byte data for debugging
  Serial.print("[FMSUART2] Received Data: ");
  for (int i = 0; i < len; i++) {
    Serial.print(data[i], HEX);  // Print each byte in hex format
    Serial.print(" ");
  }
  Serial.println();

  // Example of how to process the data, depending on your protocol
  // FMS_LOG_DEBUG("[FMSUART2] Received : %s\n\r", data); // If data is string
  // Or process the data byte by byte

}

void fms_uart2_task(void* arg) {
  BaseType_t rc;
  while (1) {
       
        #if USE_PROTOCOL == TATSUNO
           /* user tatsuno protocol*/
        #endif
        #if USE_PROTOCOL == TOUCH     
           /* user touch prootocol */
        #endif

   vTaskDelay(pdMS_TO_TICKS(100));
  }
}
