#define SHIFT_DATA 4
#define SHIFT_CLK 3
#define SHIFT_LATCH 2
#define EEPROM_D0 5
#define EEPROM_D7 12
#define WRITE_EN 13

void setAddress(int address, bool outputEnable) {
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, (address >> 8) | (outputEnable ? 0x00 : 0x80));
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, address);

  digitalWrite(SHIFT_LATCH, LOW);
  digitalWrite(SHIFT_LATCH, HIGH);
  digitalWrite(SHIFT_LATCH, LOW);
}

byte readEEPROM(int address) {
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    pinMode(pin, INPUT);
  }
  setAddress(address, /*outputEnable*/ true);

  byte data = 0;
  for (int pin = EEPROM_D7; pin >= EEPROM_D0; pin -= 1) {
    data = (data << 1) + digitalRead(pin);
  }
  return data;
}

void writeEEPROM(int address, byte data) {
  setAddress(address, /*outputEnable*/ false);
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    pinMode(pin, OUTPUT);
  }

  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    digitalWrite(pin, data & 1);
    data = data >> 1;
  }
  
  digitalWrite(WRITE_EN, LOW);
  delayMicroseconds(1);
  digitalWrite(WRITE_EN, HIGH);
  delay(10);
}

void printContents() {
  for (int base = 0; base <= 256; base += 16) {
    byte data[16];
    for (int offset = 0; offset <= 15; offset += 1) {
      data[offset] = readEEPROM(base + offset);
    }

    char buf[80];
    sprintf(buf, "%03x:  %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

    Serial.println(buf);
  }
}

void erase(){
  Serial.print("Erasing EEPROM");
  for (int address = 0; address <= 256; address += 1) {
    writeEEPROM(address, 0xff);

    if (address % 64 == 0) {
      Serial.print(".");
    }
  }
  Serial.println(" done");
}

void programPlex(){
  byte digits[] = { 0x7e, 0x30, 0x6d, 0x79, 0x33, 0x5b, 0x5f, 0x70, 0x7f, 0x7b };

  Serial.println("Programming ones place");
  for (int value = 0; value <= 255; value += 1) {
    writeEEPROM(value, digits[value % 10]);
  }
  return;
  Serial.println("Programming tens place");
  for (int value = 0; value <= 255; value += 1) {
    writeEEPROM(value + 256, digits[(value / 10) % 10]);
  }
  Serial.println("Programming hundreds place");
  for (int value = 0; value <= 255; value += 1) {
    writeEEPROM(value + 512, digits[(value / 100) % 10]);
  }
  Serial.println("Programming sign");
  for (int value = 0; value <= 255; value += 1) {
    writeEEPROM(value + 768, 0);
  }

  Serial.println("Programming ones place (twos complement)");
  for (int value = -128; value <= 127; value += 1) {
    writeEEPROM((byte)value + 1024, digits[abs(value) % 10]);
  }
  Serial.println("Programming tens place (twos complement)");
  for (int value = -128; value <= 127; value += 1) {
    writeEEPROM((byte)value + 1280, digits[abs(value / 10) % 10]);
  }
  Serial.println("Programming hundreds place (twos complement)");
  for (int value = -128; value <= 127; value += 1) {
    writeEEPROM((byte)value + 1536, digits[abs(value / 100) % 10]);
  }
  Serial.println("Programming sign (twos complement)");
  for (int value = -128; value <= 127; value += 1) {
    if (value < 0) {
      writeEEPROM((byte)value + 1792, 0x01);
    } else {
      writeEEPROM((byte)value + 1792, 0);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);
  digitalWrite(WRITE_EN, HIGH);
  pinMode(WRITE_EN, OUTPUT);
  Serial.begin(57600);
//fIC
  for (int value = 0; value <= 16; value += 1) {
    writeEEPROM(value, (byte)value*16-1);
  }
  
  for (int value = 0; value <= 16; value += 1) {
    Serial.println(readEEPROM(value));
  }

 // programPlex();
}

void loop() {
  // put your main code here, to run repeatedly:

}
