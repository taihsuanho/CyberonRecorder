# CyberonRecorder
This is an old-fashioned recording firmware running on Arduino UNO R3. The audio data is recorded from analog pin A1, and sent to PC through serial port at baud rate 921600. It can record 16-bit mono-channel audio at sampling rate 16kHz. The recorded data is sent in little endian manner. Each 16-bit sample data is followed by a 8-bit checksum, which is XOR of 0xFF and low byte and high byte of the sample.
Cyberon Voice Recorder running at PC can receive the audio data sent from Arduino UNO R3. You can download Cyberon Voice Recorder at 

https://tool.cyberon.com.tw/VoiceRecorder/index.php?lang=en

![Screenshot - 800x600](/Screenshot.png)
