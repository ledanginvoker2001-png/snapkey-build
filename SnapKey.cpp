#include <windows.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>

std::atomic<bool> running(true);

// Key pairs: A-D và S-W
int keyPairs[2][2] = {
    {65, 68}, // A D
    {83, 87}  // S W
};

std::atomic<bool> keyState[256];

// Random generator
std::random_device rd;
std::mt19937 gen(rd());

// Delay random 1–4ms
int getRandomDelay() {
    std::uniform_int_distribution<> dist(1, 4);
    return dist(gen);
}

// Jitter nhỏ (giảm pattern)
bool randomSkip() {
    std::uniform_int_distribution<> dist(0, 100);
    return dist(gen) < 3; // ~3% skip
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        int vkCode = p->vkCode;

        if (wParam == WM_KEYDOWN) {
            keyState[vkCode] = true;

            // Snap logic
            for (auto& pair : keyPairs) {
                if (vkCode == pair[0]) {
                    keyState[pair[1]] = false;
                }
                if (vkCode == pair[1]) {
                    keyState[pair[0]] = false;
                }
            }
        }

        if (wParam == WM_KEYUP) {
            keyState[vkCode] = false;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void UpdateLoop() {
    while (running) {

        for (int i = 0; i < 256; i++) {

            // Skip random để tránh pattern cứng
            if (randomSkip()) continue;

            if (keyState[i]) {
                keybd_event(i, 0, 0, 0);
            } else {
                keybd_event(i, 0, KEYEVENTF_KEYUP, 0);
            }
        }

        // Delay random (1–4ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(getRandomDelay()));
    }
}

int main() {
    // init state
    for (int i = 0; i < 256; i++) keyState[i] = false;

    HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);

    std::thread t(UpdateLoop);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    running = false;
    t.join();

    UnhookWindowsHookEx(hook);
    return 0;
}
