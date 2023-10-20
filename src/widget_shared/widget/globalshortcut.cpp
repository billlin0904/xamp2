#include <widget/globalshortcut.h>

#if defined(Q_OS_WIN)
#include <widget/win32/wintaskbar.h>
#include <base/platfrom_handle.h>
#include <Dbt.h>
#else
#include <Carbon/Carbon.h>
#include <widget/osx/osx.h>
#include <QMap>
#include <QHash>
#endif

XAMP_PIMPL_IMPL(GlobalShortcut)

#if defined(Q_OS_WIN)

class XAMP_WIDGET_SHARED_EXPORT GlobalShortcut::GlobalShortcutImpl {
public:
    GlobalShortcutImpl() = default;

    quint32 nativeKeycode(Qt::Key key) {
        // Here is list of keys that presumably work on most keyboard layouts.
        // Default branch is for keys that can change with keyboard layout.
        switch (key) {
        case Qt::Key_Escape:
            return VK_ESCAPE;
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            return VK_TAB;
        case Qt::Key_Backspace:
            return VK_BACK;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            return VK_RETURN;
        case Qt::Key_Insert:
            return VK_INSERT;
        case Qt::Key_Delete:
            return VK_DELETE;
        case Qt::Key_Pause:
            return VK_PAUSE;
        case Qt::Key_Print:
            return VK_PRINT;
        case Qt::Key_Clear:
            return VK_CLEAR;
        case Qt::Key_Home:
            return VK_HOME;
        case Qt::Key_End:
            return VK_END;
        case Qt::Key_Left:
            return VK_LEFT;
        case Qt::Key_Up:
            return VK_UP;
        case Qt::Key_Right:
            return VK_RIGHT;
        case Qt::Key_Down:
            return VK_DOWN;
        case Qt::Key_PageUp:
            return VK_PRIOR;
        case Qt::Key_PageDown:
            return VK_NEXT;
        case Qt::Key_F1:
            return VK_F1;
        case Qt::Key_F2:
            return VK_F2;
        case Qt::Key_F3:
            return VK_F3;
        case Qt::Key_F4:
            return VK_F4;
        case Qt::Key_F5:
            return VK_F5;
        case Qt::Key_F6:
            return VK_F6;
        case Qt::Key_F7:
            return VK_F7;
        case Qt::Key_F8:
            return VK_F8;
        case Qt::Key_F9:
            return VK_F9;
        case Qt::Key_F10:
            return VK_F10;
        case Qt::Key_F11:
            return VK_F11;
        case Qt::Key_F12:
            return VK_F12;
        case Qt::Key_F13:
            return VK_F13;
        case Qt::Key_F14:
            return VK_F14;
        case Qt::Key_F15:
            return VK_F15;
        case Qt::Key_F16:
            return VK_F16;
        case Qt::Key_F17:
            return VK_F17;
        case Qt::Key_F18:
            return VK_F18;
        case Qt::Key_F19:
            return VK_F19;
        case Qt::Key_F20:
            return VK_F20;
        case Qt::Key_F21:
            return VK_F21;
        case Qt::Key_F22:
            return VK_F22;
        case Qt::Key_F23:
            return VK_F23;
        case Qt::Key_F24:
            return VK_F24;
        case Qt::Key_Space:
            return VK_SPACE;
        case Qt::Key_Asterisk:
            return VK_MULTIPLY;
        case Qt::Key_Plus:
            return VK_ADD;
        case Qt::Key_Minus:
            return VK_SUBTRACT;
        case Qt::Key_Slash:
            return VK_DIVIDE;
        case Qt::Key_MediaNext:
            return VK_MEDIA_NEXT_TRACK;
        case Qt::Key_MediaPrevious:
            return VK_MEDIA_PREV_TRACK;
        case Qt::Key_MediaPlay:
            return VK_MEDIA_PLAY_PAUSE;
        case Qt::Key_MediaStop:
            return VK_MEDIA_STOP;
            // couldn't find those in VK_*
            //case Qt::Key_MediaLast:
            //case Qt::Key_MediaRecord:
        case Qt::Key_VolumeDown:
            return VK_VOLUME_DOWN;
        case Qt::Key_VolumeUp:
            return VK_VOLUME_UP;
        case Qt::Key_VolumeMute:
            return VK_VOLUME_MUTE;

        default:
            // Try to get virtual key from current keyboard layout or US.
            const HKL layout = GetKeyboardLayout(0);
            int vk = VkKeyScanEx(key, layout);
            if (vk == -1) {
                const HKL layoutUs = GetKeyboardLayout(0x409);
                vk = VkKeyScanEx(key, layoutUs);
            }
            return vk == -1 ? 0 : vk;
        }
    }

    quint32 nativeModifiers(Qt::KeyboardModifiers modifiers) {
        // MOD_ALT, MOD_CONTROL, (MOD_KEYUP), MOD_SHIFT, MOD_WIN
        quint32 native = 0;
        if (modifiers & Qt::ShiftModifier)
            native |= MOD_SHIFT;
        if (modifiers & Qt::ControlModifier)
            native |= MOD_CONTROL;
        if (modifiers & Qt::AltModifier)
            native |= MOD_ALT;
        if (modifiers & Qt::MetaModifier)
            native |= MOD_WIN;
        // TODO: resolve these?
        //if (modifiers & Qt::KeypadModifier)
        //if (modifiers & Qt::GroupSwitchModifier)
        return native;
    }

    bool registerShortcut(const WId wid, quint32 nativeKey, quint32 nativeMods) {
        auto hwnd = reinterpret_cast<HWND>(wid);
        return ::RegisterHotKey(hwnd, nativeMods ^ nativeKey, nativeMods, nativeKey);
    }

    bool unregisterShortcut(const WId wid, quint32 nativeKey, quint32 nativeMods) {
        auto hwnd = reinterpret_cast<HWND>(wid);
        return ::UnregisterHotKey(hwnd, nativeMods ^ nativeKey);
    }
};

#else

using Identifier = QPair<uint, uint>;

class XAMP_WIDGET_SHARED_EXPORT GlobalShortcut::GlobalShortcutImpl {
public:
    GlobalShortcutImpl() = default;

    bool unregisterShortcut(const WId wid, quint32 nativeKey, quint32 nativeMods) {
        Identifier id(nativeMods, nativeKey);
        if (!keyIDs.contains(id)) return false;

        EventHotKeyRef ref = keyRefs.take(keyIDs[id]);
        keyIDs.remove(id);
        return !UnregisterEventHotKey(ref);
    }

    bool registerShortcut(WId wid, quint32 nativeKey, quint32 nativeMods) {
        EventTypeSpec t;
        t.eventClass = kEventClassKeyboard;
        t.eventKind = kEventHotKeyPressed;
        InstallApplicationEventHandler(&HandleHotKey, 1, &t, nullptr, nullptr);

        EventHotKeyID keyID;
        keyID.signature = 'cute';
        keyID.id = ++hotKeySerial;

        EventHotKeyRef ref = 0;
        bool rv = !RegisterEventHotKey(nativeKey, nativeMods, keyID, GetApplicationEventTarget(), 0, &ref);
        if (rv)
        {
            keyIDs.insert(Identifier(nativeMods, nativeKey), keyID.id);
            keyRefs.insert(keyID.id, ref);
        }
        return rv;
    }

    quint32 nativeModifiers(Qt::KeyboardModifiers modifiers) {
        quint32 native = 0;
        if (modifiers & Qt::ShiftModifier)
            native |= shiftKey;
        if (modifiers & Qt::ControlModifier)
            native |= cmdKey;
        if (modifiers & Qt::AltModifier)
            native |= optionKey;
        if (modifiers & Qt::MetaModifier)
            native |= controlKey;
        if (modifiers & Qt::KeypadModifier)
            native |= kEventKeyModifierNumLockMask;
        return native;
    }

    quint32 nativeKeycode(Qt::Key key) {
        UTF16Char ch;
        // Constants found in NSEvent.h from AppKit.framework
        switch (key) {
        case Qt::Key_Return:
            return kVK_Return;
        case Qt::Key_Enter:
            return kVK_ANSI_KeypadEnter;
        case Qt::Key_Tab:
            return kVK_Tab;
        case Qt::Key_Space:
            return kVK_Space;
        case Qt::Key_Backspace:
            return kVK_Delete;
        case Qt::Key_Control:
            return kVK_Command;
        case Qt::Key_Shift:
            return kVK_Shift;
        case Qt::Key_CapsLock:
            return kVK_CapsLock;
        case Qt::Key_Option:
            return kVK_Option;
        case Qt::Key_Meta:
            return kVK_Control;
        case Qt::Key_F17:
            return kVK_F17;
        case Qt::Key_VolumeUp:
            return kVK_VolumeUp;
        case Qt::Key_VolumeDown:
            return kVK_VolumeDown;
        case Qt::Key_F18:
            return kVK_F18;
        case Qt::Key_F19:
            return kVK_F19;
        case Qt::Key_F20:
            return kVK_F20;
        case Qt::Key_F5:
            return kVK_F5;
        case Qt::Key_F6:
            return kVK_F6;
        case Qt::Key_F7:
            return kVK_F7;
        case Qt::Key_F3:
            return kVK_F3;
        case Qt::Key_F8:
            return kVK_F8;
        case Qt::Key_F9:
            return kVK_F9;
        case Qt::Key_F11:
            return kVK_F11;
        case Qt::Key_F13:
            return kVK_F13;
        case Qt::Key_F16:
            return kVK_F16;
        case Qt::Key_F14:
            return kVK_F14;
        case Qt::Key_F10:
            return kVK_F10;
        case Qt::Key_F12:
            return kVK_F12;
        case Qt::Key_F15:
            return kVK_F15;
        case Qt::Key_Help:
            return kVK_Help;
        case Qt::Key_Home:
            return kVK_Home;
        case Qt::Key_PageUp:
            return kVK_PageUp;
        case Qt::Key_Delete:
            return kVK_ForwardDelete;
        case Qt::Key_F4:
            return kVK_F4;
        case Qt::Key_End:
            return kVK_End;
        case Qt::Key_F2:
            return kVK_F2;
        case Qt::Key_PageDown:
            return kVK_PageDown;
        case Qt::Key_F1:
            return kVK_F1;
        case Qt::Key_Left:
            return kVK_LeftArrow;
        case Qt::Key_Right:
            return kVK_RightArrow;
        case Qt::Key_Down:
            return kVK_DownArrow;
        case Qt::Key_Up:
            return kVK_UpArrow;
        default:
            ;
        }

        if (key == Qt::Key_Escape)	ch = 27;
        else if (key == Qt::Key_Return) ch = 13;
        else if (key == Qt::Key_Enter) ch = 3;
        else if (key == Qt::Key_Tab) ch = 9;
        else ch = key;

        CFDataRef currentLayoutData;
        TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardInputSource();

        if (currentKeyboard == nullptr)
            return 0;

        currentLayoutData = (CFDataRef)TISGetInputSourceProperty(currentKeyboard, kTISPropertyUnicodeKeyLayoutData);
        CFRelease(currentKeyboard);
        if (currentLayoutData == nullptr) {//Japanese or Chinese always return null
            currentKeyboard = TISCopyCurrentKeyboardLayoutInputSource();
            currentLayoutData = (CFDataRef)TISGetInputSourceProperty(currentKeyboard, kTISPropertyUnicodeKeyLayoutData);
            if (!currentLayoutData)
                return 0;
        }

        UCKeyboardLayout* header = (UCKeyboardLayout*)CFDataGetBytePtr(currentLayoutData);
        UCKeyboardTypeHeader* table = header->keyboardTypeList;

        uint8_t* data = (uint8_t*)header;
        // God, would a little documentation for this shit kill you...
        for (quint32 i = 0; i < header->keyboardTypeCount; i++)
        {
            UCKeyStateRecordsIndex* stateRec = 0;
            if (table[i].keyStateRecordsIndexOffset != 0)
            {
                stateRec = reinterpret_cast<UCKeyStateRecordsIndex*>(data + table[i].keyStateRecordsIndexOffset);
                if (stateRec->keyStateRecordsIndexFormat != kUCKeyStateRecordsIndexFormat) stateRec = 0;
            }

            UCKeyToCharTableIndex* charTable = reinterpret_cast<UCKeyToCharTableIndex*>(data + table[i].keyToCharTableIndexOffset);
            if (charTable->keyToCharTableIndexFormat != kUCKeyToCharTableIndexFormat) continue;

            for (quint32 j = 0; j < charTable->keyToCharTableCount; j++)
            {
                UCKeyOutput* keyToChar = reinterpret_cast<UCKeyOutput*>(data + charTable->keyToCharTableOffsets[j]);
                for (quint32 k = 0; k < charTable->keyToCharTableSize; k++)
                {
                    if (keyToChar[k] & kUCKeyOutputTestForIndexMask)
                    {
                        long idx = keyToChar[k] & kUCKeyOutputGetIndexMask;
                        if (stateRec && idx < stateRec->keyStateRecordCount)
                        {
                            UCKeyStateRecord* rec = reinterpret_cast<UCKeyStateRecord*>(data + stateRec->keyStateRecordOffsets[idx]);
                            if (rec->stateZeroCharData == ch) return k;
                        }
                    }
                    else if (!(keyToChar[k] & kUCKeyOutputSequenceIndexMask) && keyToChar[k] < 0xFFFE)
                    {
                        if (keyToChar[k] == ch) return k;
                    }
                }
            }
        }

        // The code above fails to translate keys like semicolon with Qt 5.7.1.
        // Last resort is to try mapping the rest of the keys directly.
        switch (key)
        {
        case Qt::Key_A:
            return kVK_ANSI_A;
        case Qt::Key_S:
            return kVK_ANSI_S;
        case Qt::Key_D:
            return kVK_ANSI_D;
        case Qt::Key_F:
            return kVK_ANSI_F;
        case Qt::Key_H:
            return kVK_ANSI_H;
        case Qt::Key_G:
            return kVK_ANSI_G;
        case Qt::Key_Z:
            return kVK_ANSI_Z;
        case Qt::Key_X:
            return kVK_ANSI_X;
        case Qt::Key_C:
            return kVK_ANSI_C;
        case Qt::Key_V:
            return kVK_ANSI_V;
        case Qt::Key_B:
            return kVK_ANSI_B;
        case Qt::Key_Q:
            return kVK_ANSI_Q;
        case Qt::Key_W:
            return kVK_ANSI_W;
        case Qt::Key_E:
            return kVK_ANSI_E;
        case Qt::Key_R:
            return kVK_ANSI_R;
        case Qt::Key_Y:
            return kVK_ANSI_Y;
        case Qt::Key_T:
            return kVK_ANSI_T;
        case Qt::Key_1:
            return kVK_ANSI_1;
        case Qt::Key_2:
            return kVK_ANSI_2;
        case Qt::Key_3:
            return kVK_ANSI_3;
        case Qt::Key_4:
            return kVK_ANSI_4;
        case Qt::Key_6:
            return kVK_ANSI_6;
        case Qt::Key_5:
            return kVK_ANSI_5;
        case Qt::Key_Equal:
            return kVK_ANSI_Equal;
        case Qt::Key_9:
            return kVK_ANSI_9;
        case Qt::Key_7:
            return kVK_ANSI_7;
        case Qt::Key_Minus:
            return kVK_ANSI_Minus;
        case Qt::Key_8:
            return kVK_ANSI_8;
        case Qt::Key_0:
            return kVK_ANSI_0;
        case Qt::Key_BracketRight:
            return kVK_ANSI_RightBracket;
        case Qt::Key_O:
            return kVK_ANSI_O;
        case Qt::Key_U:
            return kVK_ANSI_U;
        case Qt::Key_BracketLeft:
            return kVK_ANSI_LeftBracket;
        case Qt::Key_I:
            return kVK_ANSI_I;
        case Qt::Key_P:
            return kVK_ANSI_P;
        case Qt::Key_L:
            return kVK_ANSI_L;
        case Qt::Key_J:
            return kVK_ANSI_J;
        case Qt::Key_QuoteDbl:
            return kVK_ANSI_Quote;
        case Qt::Key_K:
            return kVK_ANSI_K;
        case Qt::Key_Semicolon:
            return kVK_ANSI_Semicolon;
        case Qt::Key_Backslash:
            return kVK_ANSI_Backslash;
        case Qt::Key_Comma:
            return kVK_ANSI_Comma;
        case Qt::Key_Slash:
            return kVK_ANSI_Slash;
        case Qt::Key_N:
            return kVK_ANSI_N;
        case Qt::Key_M:
            return kVK_ANSI_M;
        case Qt::Key_Period:
            return kVK_ANSI_Period;
        case Qt::Key_Dead_Grave:
            return kVK_ANSI_Grave;
        case Qt::Key_Asterisk:
            return kVK_ANSI_KeypadMultiply;
        case Qt::Key_Plus:
            return kVK_ANSI_KeypadPlus;
        case Qt::Key_Clear:
            return kVK_ANSI_KeypadClear;
        case Qt::Key_Escape:
            return kVK_Escape;
        default:
            ;
        }

        return 0;
    }
private:
    static OSStatus HandleHotKey(EventHandlerCallRef nextHandler, EventRef event, void* data) {
        if (GetEventClass(event) == kEventClassKeyboard && GetEventKind(event) == kEventHotKeyPressed) {
            EventHotKeyID keyID;
            GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, nullptr, sizeof(keyID), nullptr, &keyID);
            Identifier id = keyIDs.key(keyID.id);
            //activateShortcut(id.second, id.first);
        }
        return noErr;
    }
private:
    static int32_t hotKeySerial;
    static QMap<quint32, EventHotKeyRef> keyRefs;
    static QHash<Identifier, quint32> keyIDs;
};

int32_t GlobalShortcut::GlobalShortcutImpl::hotKeySerial = 0;
QMap<quint32, EventHotKeyRef> GlobalShortcut::GlobalShortcutImpl::keyRefs;
QHash<Identifier, quint32> GlobalShortcut::GlobalShortcutImpl::keyIDs;
#endif

GlobalShortcut::GlobalShortcut()
    : impl_(MakeAlign<GlobalShortcutImpl>()) {
}

bool GlobalShortcut::RegisterShortcut(const WId wid, quint32 native_key, quint32 native_mods) {
    return impl_->registerShortcut(wid, native_key, native_mods);
}

bool GlobalShortcut::UnregisterShortcut(const WId wid, quint32 native_key, quint32 native_mods) {
    return impl_->unregisterShortcut(wid, native_key, native_mods);
}

quint32 GlobalShortcut::NativeModifiers(Qt::KeyboardModifiers modifiers) {
    return impl_->nativeModifiers(modifiers);
}

quint32 GlobalShortcut::NativeKeycode(Qt::Key key) {
    return impl_->nativeKeycode(key);
}

