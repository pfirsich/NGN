http://gamedev.stackexchange.com/questions/12146/polling-vs-event-driven-input
http://gamedev.stackexchange.com/questions/3076/keyboard-input-system-handling
http://www.gamedev.net/blog/355/entry-2250186-designing-a-robust-input-handling-system-for-games/
http://www.gamedev.net/page/resources/_/technical/general-programming/asynchronous-keyboard-input-for-fixed-time-step-games-r3959

```c++
class InputContext {
public:
    enum class BinaryInputEventType {
        DOWN, UP
    };
    using Scancode = uint32_t;

    using LowLevelKeyboardInputSignalType = Signal<void(Scancode, BinaryInputEventType)>;
    LowLevelKeyboardInputSignalType lowLevelKeyboardInputSignal;

    using LowLevelMouseMoveSignalType = Signal<void(int, int)>;
    LowLevelMouseMoveSignalType lowLevelMouseMoveSignal;

    LowLevelMouseButtonSignalType lowLevelMouseButtonSignal;

    void registerHighlevelActionInput // events, pressed, released, etc.
    void registerHighlevelStateInput // a state
    void registerHighlevelRangeInput // axes
};
```

/* contexts.yml
contexts:
    mainloop:
        moveX:
            type: KeyboardRangeInput
            positive: D
            negative: A
            translation: s-curve
        moveZ:
            type: KeyboardRangeInput
            positive: S
            negative: W
        shoot:
            type: MouseButtonInput
            button: 1
 */