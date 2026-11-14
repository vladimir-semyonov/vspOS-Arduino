const byte BUF_SIZE = 32;   // размер буфера: 31 символ команды + 1 под завершающий ноль
char buf[BUF_SIZE];         // сюда складываем набираемую строку
byte len = 0;               // сколько символов сейчас в буфере

const byte LED_PIN = LED_BUILTIN;        // встроенный светодиод (пин 13 на Nano и Uno)
const unsigned int BLINK_MS = 500;       // период переключения при мигании
bool blinking = false;                   // включён ли режим мигания
unsigned long lastToggle = 0;            // когда в последний раз переключили светодиод

extern int __heap_start, *__brkval;      // служебные переменные менеджера памяти AVR
void (*resetFunc)(void) = 0;             // "функция" по адресу 0, то есть начало программы

void printPrompt() {
  Serial.print(F("> "));    // приглашение, без перевода строки
}

// Печатает число минимум из двух цифр: 5 -> 05
void print2(unsigned long n) {
  if (n < 10) Serial.print('0');
  Serial.print(n);
}
// Сколько байт ОЗУ свободно: расстояние между кучей и стеком
int freeRam() {
  int v;                                 // локальная переменная лежит на вершине стека
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

void cmdHelp() {
  Serial.println(F("Команды:"));
  Serial.println(F("  help - список команд"));
  Serial.println(F("  uptime - время работы системы"));
  Serial.println(F("  mem    - свободная память"));
  Serial.println(F("  led    - led on | off | blink"));
  Serial.println(F("  reboot - перезагрузка"));
}

void cmdUptime() {
  unsigned long sec = millis() / 1000;   // миллисекунды -> секунды
  unsigned long h = sec / 3600;          // целые часы
  unsigned long m = (sec % 3600) / 60;   // минуты без учёта часов
  unsigned long s = sec % 60;            // оставшиеся секунды
  Serial.print(F("Время с начала работы системы: "));
  print2(h);
  Serial.print(':');
  print2(m);
  Serial.print(':');
  print2(s);
  Serial.println();
}

void cmdMem() {
  Serial.print(F("Свободно ОЗУ: "));
  Serial.print(freeRam());
  Serial.println(F(" байт из 2048"));
}

void cmdReboot() {
  Serial.println(F("Перезагрузка..."));
  Serial.flush();                        // дождаться, пока текст полностью уйдёт в порт
  resetFunc();                           // прыжок в начало программы
}

void ledUsage() {
  Serial.println(F("Использование: led on | off | blink"));
}

void cmdLed(const char* arg) {
  if (arg == NULL) {                     // NULL значит "аргумента нет"
    ledUsage();
    return;
  }
  if (strcmp(arg, "on") == 0) {
    blinking = false;                    // выключаем мигание, иначе оно перебьёт "on"
    digitalWrite(LED_PIN, HIGH);
    Serial.println(F("LED включён"));
  } else if (strcmp(arg, "off") == 0) {
    blinking = false;
    digitalWrite(LED_PIN, LOW);
    Serial.println(F("LED выключен"));
  } else if (strcmp(arg, "blink") == 0) {
    blinking = true;
    lastToggle = millis();               // отсчёт периода начинаем с этого момента
    Serial.println(F("LED мигает"));
  } else {
    ledUsage();
  }
}

// Разбор команды: сравниваем введённую строку с известными
void execute(char* line) {
  char* cmd = strtok(line, " ");         // отрезаем первое слово
  char* arg = strtok(NULL, " "); // следующее слово (или NULL, если его нет)
  Serial.print(F("[cmd="));
  Serial.print(cmd);
  Serial.print(F("] [arg="));
  Serial.print(arg);          // если аргумента нет, выведется пусто
  Serial.println(F("]"));
  if (cmd == NULL) return;               // в строке были одни пробелы

  if (strcmp(cmd, "help") == 0) {
    cmdHelp();
  } else if (strcmp(cmd, "uptime") == 0) {
    cmdUptime();
  } else if (strcmp(cmd, "mem") == 0) {
    cmdMem();
  } else if (strcmp(cmd, "led") == 0) {
    cmdLed(arg);
  } else if (strcmp(cmd, "reboot") == 0) {
    cmdReboot();
  } else {
    Serial.println(F("Неизвестная команда. Введите help"));
  }
}

void setup() {
  Serial.begin(9600);               // скорость связи с компьютером
  Serial.println(F("*********************"));
  Serial.println(F("    vspOS v0.0.3"));
  Serial.println(F("*********************"));
  Serial.println(F("Введите help для списка команд"));
  printPrompt();
}

void loop() {
  // 1. Читаем ввод
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (len > 0) {
        buf[len] = '\0';
        Serial.println(buf);
        execute(buf);
        len = 0;
        printPrompt();
      }
    } else if (len < BUF_SIZE - 1) {
      buf[len++] = c;
    }
  }

  // 2. Фоновая задача: мигание без delay()
  if (blinking && millis() - lastToggle >= BLINK_MS) {
    lastToggle = millis();
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));  // меняем состояние на противоположное
  }
}
