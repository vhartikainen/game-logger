#include "apmlog.h"

#include <algorithm>
#include <cmath>

// #define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX                        // Prevent windows.h min/max macros clashing with std::min/std::max
#include <Windows.h>
#include <Xinput.h>
#include <psapi.h>
#include <Winbase.h>

#include "common.h"

int keyboardCounter = 0;
HHOOK keyboardHook;

int mouseCounter = 0;
HHOOK mouseHook;

int gamepadCounter = 0;
XINPUT_CAPABILITIES gamepadCapabilities;
XINPUT_STATE *gamepadState = new XINPUT_STATE();
XINPUT_STATE *gamepadPrevState = new XINPUT_STATE();


LRESULT CALLBACK keyboardCounterProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (wParam == WM_KEYUP)
        ++keyboardCounter;
    return CallNextHookEx(0, nCode, wParam, lParam);
}

LRESULT CALLBACK mouseCounterProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if ((wParam == WM_LBUTTONDOWN) || (wParam == WM_RBUTTONDOWN))
        ++mouseCounter;
    return CallNextHookEx(0, nCode, wParam, lParam);
}


APMLog::APMLog(qint32 bufferSize, qint32 serverTime)
{    
    // Set up APM counting
    this->bufferSize = bufferSize;
    buffer = new int[bufferSize];

    // Synchronize buffers between clients.
    // Since the clock on server side and the apm buffer size is common to all clients,
    // and each client sends its apm data when the ringbuffer is full
    // we only need to set the initial buffer position to the modulus of serverTime to
    // make the clients send their data during the same second
    bufferPos = serverTime % bufferSize; // Synchronize clients to send updates within the same second.

    // Initialize all variables to zero
    secPos = 0;
    current = 0;
    lastMouseCount = 0;
    lastKeyboardCount = 0;
    lastGamepadCount = 0;
    for (int i = 0; i < 60; ++i) {
        sec[i] = 0;
    }
    for (int i = 0; i < bufferSize; ++i) {
        buffer[i] = 0;
    }

    // Start low-level keyboard and mouse hooks to trap key and mouse presses
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardCounterProc, GetModuleHandle(0), 0);
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseCounterProc, GetModuleHandle(0), 0);

    gamepadIndex = -1;
    gamepadCheckCounter = 0;

    // Setup frame timer for reading gamepad state
}

APMLog::~APMLog()
{
    // Detach hooks
    if (keyboardHook)
        UnhookWindowsHookEx(keyboardHook);
    if (mouseHook)
        UnhookWindowsHookEx(mouseHook);
}


bool APMLog::update()
{
    QDEBUG("[APMLog::update()] called");

    // Get actions after last call to this function. Store to local variables so that the values wouldn't change
    // between two successive accesses
    int keyb = keyboardCounter;
    int mouse = mouseCounter;
    int gamepad = gamepadCounter;
    int diff = (keyb - lastKeyboardCount) + (mouse - lastMouseCount) + (gamepad - lastGamepadCount);
    lastKeyboardCount = keyb;
    lastMouseCount = mouse;
    lastGamepadCount = gamepad;

    // Update action count buffer
    secPos = (secPos + 1) % 60;
    current += diff - sec[secPos];
    sec[secPos] = diff;

    // Update APM buffer
    bufferPos = (bufferPos + 1) % bufferSize;
    buffer[bufferPos] = current;

    if (bufferPos == (bufferSize-1)) {
        // Buffer is filled with new APM data
        QDEBUG("[APMLog::update()] exiting with buffer full and current apm=%d", current);
        return true;
    } else {
        QDEBUG("[APMLog::update()] exiting (buffer not full) and current apm=%d", current);
        return false;
    }

}

void APMLog::updateGamepad()
{
    if (gamepadIndex < 0) {
        // No gamepad connected. Enumerate all.

        // Don't check too often.
        if (gamepadCheckCounter > GAMEPAD_SEARCH_INTERVAL_SEC*GAMEPAD_CHECK_FPS)
            gamepadCheckCounter = 0;

        if (gamepadCheckCounter == 0) {
            // Iterate through all gamepads and check if we can get their capabilities. If we can, we've found a gamepad.
            for (int i = 0; i < 4; ++i) {
                if (XInputGetCapabilities(i, XINPUT_FLAG_GAMEPAD, &gamepadCapabilities) == ERROR_SUCCESS) {
                    // Gamepad found
                    QDEBUG("[APMLog::updateGamepad()] found gamepad at index %d", i);
                    gamepadIndex = i;
                    break;
                }
            }

        }

        if (gamepadIndex < 0) {
            // Gamepad not found or we are waiting to check again.
            ++gamepadCheckCounter;
            return;
        }

    }

    if (XInputGetState(gamepadIndex, gamepadState) != ERROR_SUCCESS)
    {
        // Device is no longer connected
        gamepadIndex = -1;
        gamepadCheckCounter = 0;
    } else if (lastGamepadPacket != gamepadState->dwPacketNumber) {
        // Left thumb changes
        float leftXDiff = gamepadState->Gamepad.sThumbLX - gamepadPrevState->Gamepad.sThumbLX;
        float leftYDiff = gamepadState->Gamepad.sThumbLY - gamepadPrevState->Gamepad.sThumbLY;
        float leftMagnitude = std::min(std::sqrt(leftXDiff*leftXDiff + leftYDiff*leftYDiff)/32767,1.0f);

        // Right thumb changes
        float rightXDiff = gamepadState->Gamepad.sThumbRX - gamepadPrevState->Gamepad.sThumbRX;
        float rightYDiff = gamepadState->Gamepad.sThumbRY - gamepadPrevState->Gamepad.sThumbRY;
        float rightMagnitude = std::min(std::sqrt(rightXDiff*rightXDiff + rightYDiff*rightYDiff)/32767,1.0f);

        // Shoulder buttons
        float leftShoulder = std::min(std::abs((float)gamepadState->Gamepad.bLeftTrigger - gamepadPrevState->Gamepad.bLeftTrigger)/255,1.0f);
        float rightShoulder = std::min(std::abs((float)gamepadState->Gamepad.bRightTrigger - gamepadPrevState->Gamepad.bRightTrigger)/255,1.0f);

        QDEBUG("[APMLog::updateGamepad()] leftMagnitude=%g, rightMagnitude=%g, leftShoulder=%g, rightShoulder=%g, oldButtons=%d, newButtons=%d", leftMagnitude, rightMagnitude, leftShoulder, rightShoulder, gamepadPrevState->Gamepad.wButtons,  gamepadState->Gamepad.wButtons);

        bool analogChange = (leftMagnitude > GAMEPAD_RELATIVE_MIN_CHANGE) ||
                (rightMagnitude > GAMEPAD_RELATIVE_MIN_CHANGE) ||
                (leftShoulder > GAMEPAD_RELATIVE_MIN_CHANGE) ||
                (rightShoulder > GAMEPAD_RELATIVE_MIN_CHANGE);

        // Check if state has changed
        if (analogChange || (gamepadState->Gamepad.wButtons != gamepadPrevState->Gamepad.wButtons)) {

            if (analogChange || (gamepadState->Gamepad.wButtons & ~gamepadPrevState->Gamepad.wButtons)) {
                // Gamepad status has changed. Exclude button release events.
                ++gamepadCounter;
            }

            // Switch only if a change is detected, since now we can detect the distance from last changed position.
            XINPUT_STATE *temp = gamepadPrevState;
            gamepadPrevState = gamepadState;
            gamepadState = temp;
        }
        lastGamepadPacket = gamepadState->dwPacketNumber;
    }

}
