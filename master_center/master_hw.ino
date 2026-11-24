/*
 * master_hw.ino  -  입출력 함수 (LED, 지게차 송신, 폰 송신 큐)
 * 설정/상태/상수는 master_center.ino 에, 판단은 master_logic.ino 에 있다.
 */

void ledsOff() {  // ◆ NEW
  const uint8_t offLevel = (LED_ON_LEVEL == HIGH) ? LOW : HIGH;
  for (uint8_t i = 0; i < 4; i++) digitalWrite(LED_PINS_CW[i], offLevel);
}

// idx: LED_PINS_CW 순서 번호. 한 번에 1개만 켠다 (사용자 확인).
void setLedIndex(int8_t idx) {  // ◆ NEW
  ledsOff();
  if (idx >= 0 && idx < 4) digitalWrite(LED_PINS_CW[idx], LED_ON_LEVEL);
}

void initMasterHw() {  // ◆ NEW
  for (uint8_t i = 0; i < 4; i++) pinMode(LED_PINS_CW[i], OUTPUT);
  ledsOff();
  Serial.begin(LINK_BAUD);   // 지게차 링크 (하드웨어 시리얼)
  phone.begin(PHONE_BAUD);   // 폰 링크 (소프트웨어 시리얼)
}

void sendToForklift(char c) {  // ◆ NEW: 1바이트, 응답 없음
  Serial.write(c);
}

// ---------------- 폰 메시지 ----------------
// 문구는 모두 16자 이내 영문 (16x2 LCD 차선책에도 그대로 사용 가능, 플래시 절약)
const __FlashStringHelper *msgText(uint8_t id) {  // ◆ NEW
  switch (id) {
    case MSG_SEL_LOAD:    return F("SEL LOAD");
    case MSG_SEL_UNLOAD:  return F("SEL UNLOAD");
    case MSG_EMPTY:       return F("EMPTY");
    case MSG_BUSY:        return F("BUSY");
    case MSG_JOB:         return F("JOB");
    case MSG_ST_WAIT:     return F("ST WAIT");
    case MSG_ST_TO_LOAD:  return F("ST TO LOAD");
    case MSG_ST_APPROACH: return F("ST APPROACH");
    case MSG_ST_LIFT_UP:  return F("ST LIFT UP");
    case MSG_ST_REVERSE:  return F("ST REVERSE");
    case MSG_ST_TO_UNLOAD: return F("ST TO UNLOAD");
    case MSG_ST_UNLOAD:   return F("ST UNLOAD");
    case MSG_ST_RETURN:   return F("ST RETURN");
    case MSG_ST_DONE:     return F("ST DONE");
    case MSG_EMERGENCY:   return F("EMERGENCY");
    case MSG_LINE_LOST:   return F("LINE LOST");
    case MSG_JOB_RUNNING: return F("JOB RUNNING");
    default:              return F("");
  }
}

// arg1/arg2: 문구 뒤에 붙일 글자 (0 이면 생략). JOB 은 "JOB 3>B" 형식.
void queuePhone(uint8_t id, char arg1, char arg2) {  // ◆ NEW
  if (queueCount >= QUEUE_SIZE) return;   // 가득 차면 새 메시지를 버림
  uint8_t tail = (queueHead + queueCount) % QUEUE_SIZE;
  queueId[tail] = id;
  queueArg1[tail] = arg1;
  queueArg2[tail] = arg2;
  queueCount++;
}

void phoneSendEntry(uint8_t id, char arg1, char arg2) {  // ◆ NEW
  phone.print(msgText(id));
  if (arg1 != 0) { phone.write(' '); phone.write(arg1); }
  if (arg2 != 0) { phone.write('>'); phone.write(arg2); }
  phone.write('\n');
}

// 지게차 링크로 받을 바이트가 없을 때만 한 줄 송신 (수신 손실 방지, 제안)
void flushPhoneQueue() {  // ◆ NEW
  if (queueCount == 0 || Serial.available() > 0) return;
  phoneSendEntry(queueId[queueHead], queueArg1[queueHead], queueArg2[queueHead]);
  queueHead = (queueHead + 1) % QUEUE_SIZE;
  queueCount--;
}
