#include <stdio.h>
#include <unistd.h>
// 외부 함수 선언
void show_message_on_lcd(const char* message);
int main() {
    show_message_on_lcd("Clear!");
    sleep(5); // 5초 동안 메시지를 유지
    return 0;
}
