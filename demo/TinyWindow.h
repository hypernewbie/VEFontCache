//created by Ziyad Barakat 2014 - 2020

#ifndef TINYWINDOW_H
#define TINYWINDOW_H

#if defined(_WIN32) || defined(_WIN64)
#define TW_WINDOWS
#if defined(_MSC_VER)
//this automatically loads the OpenGL library if you are using Visual studio. feel free to comment out
#pragma comment (lib, "opengl32.lib")
//for gamepad support
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "xinput.lib")
//this makes sure that the entry point of your program is main() not Winmain(). feel free to comment out
#if defined(TW_NO_CONSOLE)
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#else
#pragma comment(linker, "/subsystem:console /ENTRY:mainCRTStartup")
#endif
#endif //_MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif //WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX 1
#endif //NOMINMAX
#ifndef WGL_WGLEXT_PROTOTYPES
#define WGL_WGLEXT_PROTOTYPES 1
#endif //WGL_WGLEXT_PROTOTYPES


#include <Windows.h>
#if !defined(TW_USE_VULKAN)
#include <gl/GL.h>
#include <gl/wglext.h>
#include <dinput.h>
#include <Xinput.h>
#include <Dbt.h>
#else
#include <vulkan.h>
#endif
#include <io.h>
#include <fcntl.h>
#include <mmsystem.h>
#include <shellapi.h>
#endif  //_WIN32 || _WIN64

#if defined(__linux__)
#define TW_LINUX
#if !defined(TW_USE_VULKAN)
#include <GL/glx.h>
#else
#include <vulkan.h>
#endif
#include <X11/X.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#endif //__linux__

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <stack>
#include <climits>
#include <cstring>
#include <functional>
#include <memory>
#include <system_error>
#include <bitset>
#include <cctype>
#include <algorithm>
#include <limits>

namespace TinyWindow
{
    class tWindow;
    const int defaultWindowWidth = 1280;
    const int defaultWindowHeight = 720;

    template<typename type>
    struct vec2_t
    {
        vec2_t()
        {
            this->x = 0;
            this->y = 0;
        }

        vec2_t(type x, type y)
        {
            this->x = x;
            this->y = y;
        }

        union
        {
            type x;
            type width;
        };

        union
        {
            type y;
            type height;
        };

        static vec2_t Zero()
        {
            return vec2_t<type>(0, 0);
        }
    };

    template<typename type>
    struct vec4_t
    {
        vec4_t()
        {
            this->x = 0;
            this->y = 0;
            this->z = 0;
            this->w = 0;
        }

        vec4_t(type x, type y, type z, type w)
        {
            this->x = x;
            this->y = y;
            this->z = z;
            this->w = w;
        }

        union
        {
            type x;
            type width;
            type left;
        };

        union
        {
            type y;
            type height;
            type top;
        };

        union
        {
            type z;
            type depth;
            type right;
        };

        union 
        {
            type w;
            type homogenous;
            type bottom;
        };

        static vec4_t Zero()
        {
            return vec4_t<type>(0, 0, 0, 0);
        }
    };

    struct monitorSetting_t
    {
        vec2_t<unsigned int>        resolution; //native resolution?
        unsigned int                bitsPerPixel;
        unsigned int                displayFrequency;

#if defined(TW_WINDOWS)
        unsigned int       displayFlags;
        unsigned int       fixedOutput;
#endif

        monitorSetting_t(vec2_t<unsigned int> inResolution = vec2_t<unsigned int>().Zero(), 
            unsigned int inBitsPerPixel = 0, unsigned int inDisplayFrequency = 0) : 
            resolution(inResolution), bitsPerPixel(inBitsPerPixel), displayFrequency(inDisplayFrequency)
        {

#if defined(TW_WINDOWS)
            displayFlags = 0;
            fixedOutput = 0;
#endif
        }
    };  

    struct monitor_t
    {
        monitorSetting_t*                   currentSetting;
        std::vector<monitorSetting_t*>      settings;
        //store all display settings

        vec2_t<unsigned int> resolution;
        vec4_t<int> extents;
        std::string deviceName;
        std::string monitorName;
        std::string displayName;
        bool isPrimary;

    //private:
#if defined(TW_WINDOWS)
        HMONITOR monitorHandle;
#elif defined(TW_LINUX)

#endif

        monitor_t() 
        {
            currentSetting = nullptr;
            isPrimary = false;
            resolution = vec2_t<unsigned int>::Zero();
            extents = vec4_t<int>::Zero();
#if defined(TW_WINDOWS)
            monitorHandle = nullptr;
#endif
        };

        monitor_t(std::string displayName, std::string deviceName, std::string monitorName, bool isPrimary = false)
        {
            //this->resolution = resolution;
            //this->extents = extents;
#if defined(TW_WINDOWS)
            this->monitorHandle = nullptr;
            this->displayName = displayName;
#endif
            this->currentSetting = nullptr;         
            this->deviceName = deviceName;
            this->monitorName = monitorName;
            this->isPrimary = isPrimary;
        }
    };

    struct formatSetting_t
    {
        friend class windowManager;

        int redBits;
        int greenBits;
        int blueBits;
        int alphaBits;
        int depthBits;
        int stencilBits;

        int accumRedBits;
        int accumGreenBits;
        int accumBlueBits;
        int accumAplhaBits;

        int auxBuffers;
        int numSamples;

        bool stereo;
        bool doubleBuffer;
        bool pixelRGB;

    private:
#if defined(TW_WINDOWS)
        int handle;
#endif

    public:

        formatSetting_t(int redBits = 8, int greenBits = 8, int blueBits = 8, int alphaBits = 8, 
            int depthBits = 32, int stencilBits = 8, 
            int accumRedBits = 0, int accumGreenBits = 0, int accumBlueBits = 0, int accumAplhaBits = 0, 
            int auxBuffers = 0, int numSamples = 0, bool stereo = false, bool doubleBuffer = true) 
        {
            this->redBits = redBits;
            this->greenBits = greenBits;
            this->blueBits = blueBits;
            this->alphaBits = alphaBits;
            this->depthBits = depthBits;
            this->stencilBits = stencilBits;

            this->accumRedBits = accumRedBits;
            this->accumGreenBits = accumGreenBits;
            this->accumBlueBits = accumBlueBits;
            this->accumAplhaBits = accumAplhaBits;

            this->auxBuffers = auxBuffers;
            this->numSamples = numSamples;

            this->stereo = stereo;
            this->doubleBuffer = doubleBuffer;
            pixelRGB = true;
#if defined(TW_WINDOWS)
            this->handle = 0;
#endif
        }
    };

    enum class profile_t
    {
        core,
        compatibility,
    };

    enum class state_t
    {
        normal,                                 /**< The window is in its default state */
        maximized,                              /**< The window is currently maximized */
        minimized,                              /**< The window is currently minimized */
        fullscreen,                             /**< The window is currently full screen */
    };

    struct windowSetting_t
    {
        friend class windowManager;

        windowSetting_t(const char* name = nullptr, void* userData = nullptr,
            vec2_t<unsigned int> resolution = vec2_t<unsigned int>(defaultWindowWidth, defaultWindowHeight),
            int versionMajor = 4, int versionMinor = 5, unsigned int colorBits = 8, unsigned int depthBits = 24, unsigned int stencilBits = 8,
            state_t currentState = state_t::normal, profile_t profile = profile_t::core)
        {
            this->name = name;
            this->resolution = resolution;
            this->colorBits = colorBits;
            this->depthBits = depthBits;
            this->stencilBits = stencilBits;
            this->currentState = currentState;
            this->userData = userData;
            this->versionMajor = versionMajor;
            this->versionMinor = versionMinor;
            this->enableSRGB = false;

            SetProfile(profile);
        }

        void SetProfile(profile_t profile)
        {
#if defined(TW_WINDOWS) && !defined(TW_USE_VULKAN) 
            this->profile = (profile == profile_t::compatibility) ? WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB : WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
#endif
        }

        const char*                             name;                                                   /**< Name of the window */
        int                                     colorBits;                                              /**< Color format of the window. (defaults to 32 bit color) */
        int                                     depthBits;                                              /**< Size of the Depth buffer. (defaults to 8 bit depth) */
        int                                     stencilBits;                                            /**< Size of the stencil buffer, (defaults to 8 bit) */
        TinyWindow::vec2_t<unsigned int>        resolution;                                             /**< Resolution/Size of the window stored in an array */
        void*                                   userData;
        state_t                                 currentState;                                           /**< The current state of the window. these states include Normal, Minimized, Maximized and Full screen */
        bool                                    enableSRGB;                                             /**< whether the window will support an sRGB colorspace backbuffer*/

#if defined(TW_WINDOWS) && !defined(TW_USE_VULKAN) 
        GLint                                   versionMajor;                                           /**< Major OpenGL version*/
        GLint                                   versionMinor;                                           /**< Minor OpenGL version*/
    private:
        GLint                                   profile;                                                /**< Compatibility or core OpenGL profiles*/
#endif

    };

    enum class keyState_t
    {
        bad,                                    /**< If get key state fails (could not name it ERROR) */
        up,                                     /**< The key is currently up */
        down,                                   /**< The key is currently down */
    };

    enum key_t
    {
        bad = -1,                               /**< The key pressed is considered invalid */
        first = 256 + 1,                        /**< The first key that is not a char */
        F1,                                     /**< The F1 key */
        F2,                                     /**< The F2 key */
        F3,                                     /**< The F3 key */
        F4,                                     /**< The F4 key */
        F5,                                     /**< The F5 key */
        F6,                                     /**< The F6 key */
        F7,                                     /**< The F7 key */
        F8,                                     /**< The F8 key */
        F9,                                     /**< The F9 key */
        F10,                                    /**< The F10 key */
        F11,                                    /**< The F11 key */
        F12,                                    /**< The F12 key */
        capsLock,                               /**< The CapsLock key */
        leftShift,                              /**< The left Shift key */
        rightShift,                             /**< The right Shift key */
        leftControl,                            /**< The left Control key */
        rightControl,                           /**< The right Control key */
        leftWindow,                             /**< The left Window key */
        rightWindow,                            /**< The right Window key */
        leftAlt,                                /**< The left Alternate key */
        rightAlt,                               /**< The right Alternate key */
        enter,                                  /**< The Enter/Return key */
        printScreen,                            /**< The PrintScreen key */
        scrollLock,                             /**< The ScrollLock key */
        numLock,                                /**< The NumLock key */
        pause,                                  /**< The pause/break key */
        insert,                                 /**< The insert key */
        home,                                   /**< The Home key */
        end,                                    /**< The End key */
        pageUp,                                 /**< The PageUp key */
        pageDown,                               /**< The PageDown key */
        arrowDown,                              /**< The ArrowDown key */
        arrowUp,                                /**< The ArrowUp key */
        arrowLeft,                              /**< The ArrowLeft key */
        arrowRight,                             /**< The ArrowRight key */
        keypadDivide,                           /**< The KeyPad Divide key */
        keypadMultiply,                         /**< The Keypad Multiply key */
        keypadSubtract,                         /**< The Keypad Subtract key */
        keypadAdd,                              /**< The Keypad Add key */
        keypadEnter,                            /**< The Keypad Enter key */
        keypadPeriod,                           /**< The Keypad Period/Decimal key */
        keypad0,                                /**< The Keypad 0 key */
        keypad1,                                /**< The Keypad 1 key */
        keypad2,                                /**< The Keypad 2 key */
        keypad3,                                /**< The Keypad 3 key */
        keypad4,                                /**< The Keypad 4 key */
        keypad5,                                /**< The Keypad 5 key */
        keypad6,                                /**< The Keypad 6 key */
        keypad7,                                /**< The Keypad 7 key */
        keypad8,                                /**< The keypad 8 key */
        keypad9,                                /**< The Keypad 9 key */
        backspace,                              /**< The Backspace key */
        tab,                                    /**< The Tab key */
        del,                                    /**< The Delete key */
        spacebar,                               /**< The Spacebar key */
        escape,                                 /**< The Escape key */
        apps,                                   /**< The Applications key*/
        last = escape,                          /**< The last key to be supported */
    };

    enum class buttonState_t
    {
        up,                                     /**< The mouse button is currently up */
        down                                    /**< The mouse button is currently down */
    };

    enum class mouseButton_t
    {
        left,                                   /**< The left mouse button */
        right,                                  /**< The right mouse button */
        middle,                                 /**< The middle mouse button / ScrollWheel */
        last,                                   /**< The last mouse button to be supported */
    };

    enum class mouseScroll_t
    {
        down,                                   /**< The mouse wheel up */
        up                                      /**< The mouse wheel down */
    };

    enum decorator_t
    {
        titleBar =          1L << 1,            /**< The title bar decoration of the window */
        icon =              1L << 2,            /**< The icon decoration of the window */
        border =            1L << 3,            /**< The border decoration of the window */
        minimizeButton =    1L << 4,            /**< The minimize button decoration of the window */
        maximizeButton =    1L << 5,            /**< The maximize button decoration pf the window */
        closeButton =       1L << 6,            /**< The close button decoration of the window */
        sizeableBorder =    1L << 7,            /**< The sizable border decoration of the window */
    };

    enum class style_t
    {
        bare,                                   /**< The window has no decorators but the window border and title bar */
        normal,                                 /**< The default window style for the respective platform */
        popup,                                  /**< The window has no decorators */
    };

    struct gamepad_t
    {
        enum button_t
        {
            face_top = 0,
            face_left,
            face_right,
            face_bottom,
            start,
            select,
            Dpad_top, //dpad might need to be something different?
            Dpad_left,
            Dpad_right,
            Dpad_bottom,
            left_stick,
            right_stick,
            left_shoulder,
            right_shoulder,
            special1,
            special2,
            last = special2
        };

        gamepad_t()
        {
            buttonStates.resize(last, false);

            leftStick = std::vector<float>{ 0, 0 };
            rightStick = std::vector<float>{ 0, 0 };

            leftTrigger = 0;
            rightTrigger = 0;
        }

        float leftTrigger;
        float rightTrigger;

        std::vector<float> leftStick;
        std::vector<float> rightStick;

        std::vector<bool> buttonStates;

        bool isWireless; //does X11 even pick up any of this shit?

        unsigned short ID; //how to make this a more dynamic ID system for hot-plugging?

        //we need hb_face buttons
        //we need triggers
        //we need the axis buttons as well

        //a vector of bools for buttons with each button being an index?

        //ok get and set the dead zones for each axis of the sticks and the 
        //deadzones for triggers
        //

        //clamped between 0-1
        /*void vibrate(float strength)
        {
            //

            //get the maximum vibration of that controller
            //lerp between the min and max? inverse lerp
            std::clamp<float>(strength, 0.0f, 1.0f);
            short vibrateStrength = 0;
            vibrateStrength = Lerp<short>(0, leftMotor, vibrateStrength);

        }*/

    private:

#if defined(TW_WINDOWS)

        short leftMotor;
        short rightMotor;
#endif

        template<typename t> inline t Lerp(t first, t second, t delta)
        {
            return (1 - delta) * first + delta * second;
        }

    };

    enum class error_t
    {
        success,                                /**< If a function call was successful*/
        invalidWindowName,                      /**< If an invalid window name was given */
        invalidIconPath,                        /**< If an invalid icon path was given */
        invalidWindowIndex,                     /**< If an invalid window index was given */
        invalidWindowState,                     /**< If an invalid window state was given */
        invalidResolution,                      /**< If an invalid window resolution was given */
        invalidContext,                         /**< If the OpenGL context for the window is invalid */
        existingContext,                        /**< If the window already has an OpenGL context */
        notInitialized,                         /**< If the window is being used without being initialized */
        alreadyInitialized,                     /**< If the window was already initialized */
        invalidTitlebar,                        /**< If the Title-bar text given was invalid */
        invalidCallback,                        /**< If the given event callback was invalid */
        windowInvalid,                          /**< If the window given was invalid */
        invalidWindowStyle,                     /**< If the window style gives is invalid */
        invalidVersion,                         /**< If an invalid OpenGL version is being used */
        invalidProfile,                         /**< If an invalid OpenGL profile is being used */
        invalidInterval,                        /**< If a window swap interval setting is invalid */
        fullscreenFailed,                       /**< If setting the window to fullscreen has failed */
        noExtensions,                           /**< If platform specific window extensions have not been properly loaded */
        invalidExtension,                       /**< If a platform specific window extension is not supported */
        invalidDummyWindow,                     /**< If the dummy window creation has failed */
        invalidDummyPixelFormat,                /**< If the pixel format for the dummy context id invalid */
        dummyCreationFailed,                    /**< If the dummy context has failed to be created */
        invalidDummyContext,                    /**< If the dummy context in invalid */
        dummyCannotMakeCurrent,                 /**< If the dummy cannot be made the current context */
        invalidMonitorSettingIndex,             /**< If the provided monitor setting index is invalid */
        functionNotImplemented,                 /**< If the function has not yet been implemented in the current version of the API */
        linuxCannotConnectXServer,              /**< Linux: If cannot connect to an X11 server */
        linuxInvalidVisualinfo,                 /**< Linux: If visual information given was invalid */
        linuxCannotCreateWindow,                /**< Linux: When X11 fails to create a new window */
        linuxFunctionNotImplemented,            /**< Linux: When the function has not yet been implemented on the Linux in the current version of the API */
        windowsCannotCreateWindows,             /**< Windows: When Win32 cannot create a window */
        windowsCannotInitialize,                /**< Windows: When Win32 cannot initialize */
        windowsFullscreenBadDualView,           /**< Windows: The system is DualView capable. whatever that means */
        windowsFullscreenBadFlags,              /**< Windows: Bad display change flags */
        windowsFullscreenBadMode,               /**< Windows: Bad display change mode */
        WindowsFullscreenBadParam,              /**< Windows: Bad display change Parameter */
        WindowsFullscreenChangeFailed,          /**< Windows: The display driver failed to implement the specified graphics mode */
        WindowsFullscreenNotUpdated,            /**< Windows: Unable to write settings to the registry */
        WindowsFullscreenNeedRestart,           /**< Windows: The computer must be restarted for the graphics mode to work */
        windowsFunctionNotImplemented,          /**< Windows: When a function has yet to be implemented on the Windows platform in the current version of the API */
    };

    using keyEvent_t = std::function<void(tWindow* window, int key, keyState_t keyState)>;
    using mouseButtonEvent_t = std::function<void(tWindow* window, mouseButton_t mouseButton, buttonState_t buttonState)>;
    using mouseWheelEvent_t = std::function<void(tWindow* window, mouseScroll_t mouseScrollDirection)>;
    using destroyedEvent_t = std::function<void(tWindow* window)>;
    using maximizedEvent_t = std::function<void(tWindow* window)>;
    using minimizedEvent_t = std::function<void(tWindow* window)>;
    using focusEvent_t = std::function<void(tWindow* window, bool isFocused)>;
    using movedEvent_t = std::function<void(tWindow* window, vec2_t<int> windowPosition)>;
    using resizeEvent_t = std::function<void(tWindow* window, vec2_t<unsigned int> windowResolution)>;
    using mouseMoveEvent_t = std::function<void(tWindow* window, vec2_t<int> windowMousePosition, vec2_t<int> screenMousePosition)>;
    using fileDropEvent_t = std::function<void(tWindow* window, std::vector<std::string> files, vec2_t<int> windowMousePosition)>;

    class errorCategory_t : public std::error_category
    {
    public:

        const char* name() const throw() override
        {
            return "tinyWindow";
        }

        /**
        * return the error message associated with the given error number
        */
        std::string message(int errorValue) const override
        {
            error_t err = (error_t)errorValue;
            switch (err)
            {
                case error_t::invalidWindowName:
                {
                    return "Error: invalid window name \n";
                }

                case error_t::invalidIconPath:
                {
                    return "Error: invalid icon path \n";
                }

                case error_t::invalidWindowIndex:
                {
                    return "Error: invalid window index \n";
                }

                case error_t::invalidWindowState:
                {
                    return "Error: invalid window state \n";
                }

                case error_t::invalidResolution:
                {
                    return "Error: invalid resolution \n";
                }

                case error_t::invalidContext:
                {
                    return "Error: Failed to create OpenGL context \n";
                }

                case error_t::existingContext:
                {
                    return "Error: context already created \n";
                }

                case error_t::notInitialized:
                {
                    return "Error: Window manager not initialized \n";
                }

                case error_t::alreadyInitialized:
                {
                    return "Error: window has already been initialized \n";
                }

                case error_t::invalidTitlebar:
                {
                    return "Error: invalid title bar name (cannot be null or nullptr) \n";
                }

                case error_t::invalidCallback:
                {
                    return "Error: invalid event callback given \n";
                }

                case error_t::windowInvalid:
                {
                    return "Error: window was not found \n";
                }

                case error_t::invalidWindowStyle:
                {
                    return "Error: invalid window style given \n";
                }

                case error_t::invalidVersion:
                {
                    return "Error: invalid OpenGL version \n";
                }

                case error_t::invalidProfile:
                {
                    return "Error: invalid OpenGL profile \n";
                }

                case error_t::fullscreenFailed:
                {
                    return "Error: failed to enter fullscreen mode \n";
                }

                case error_t::functionNotImplemented:
                {
                    return "Error: I'm sorry but this function has not been implemented yet :(\n";
                }

                case error_t::noExtensions:
                {
                    return "Error: Platform extensions have not been loaded correctly \n";
                }

                case error_t::invalidExtension:
                {
                    return "Error: Platform specific extension is not valid \n";
                }

                case error_t::invalidDummyWindow:
                {
                    return "Error: the dummy window failed to be created \n";
                }

                case error_t::invalidDummyPixelFormat:
                {
                    return "Error: the pixel format for the dummy context is invalid \n";
                }

                case error_t::dummyCreationFailed:
                {
                    return "Error: the dummy context has failed to be created \n";
                }               

                case error_t::invalidDummyContext:
                {
                    return "Error: the dummy context in invalid \n";
                }
                
                case error_t::dummyCannotMakeCurrent:
                {
                    return "Error: the dummy cannot be made the current context \n";
                }

                case error_t::invalidMonitorSettingIndex:
                {
                    return "Error: the provided monitor setting index is invalid \n";
                }

                case error_t::linuxCannotConnectXServer:
                {
                    return "Linux Error: cannot connect to X server \n";
                }

                case error_t::linuxInvalidVisualinfo:
                {
                    return "Linux Error: Invalid visual information given \n";
                }

                case error_t::linuxCannotCreateWindow:
                {
                    return "Linux Error: failed to create window \n";
                }

                case error_t::linuxFunctionNotImplemented:
                {
                    return "Linux Error: function not implemented on Linux platform yet. sorry :(\n";
                }

                case error_t::windowsCannotCreateWindows:
                {
                    return "Windows Error: failed to create window \n";
                }

                case error_t::windowsFullscreenBadDualView:
                {
                    return "Windows Error: The system is DualView capable. whatever that means \n";
                }

                case error_t::windowsFullscreenBadFlags:
                {
                    return "Windows Error: Bad display change flags \n";
                }

                case error_t::windowsFullscreenBadMode:
                {
                    return "Windows Error: Bad display change mode \n";
                }
                
                case error_t::WindowsFullscreenBadParam:
                {
                    return "Windows Error: Bad display change Parameter \n";
                }

                case error_t::WindowsFullscreenChangeFailed:
                {
                    return "Windows Error: The display driver failed to implement the specified graphics mode \n";
                }

                case error_t::WindowsFullscreenNotUpdated:
                {
                    return "Windows Error: Unable to write settings to the registry \n";
                }

                case error_t::WindowsFullscreenNeedRestart:
                {
                    return "Windows Error: The computer must be restarted for the graphics mode to work \n";
                }

                case error_t::windowsFunctionNotImplemented:
                {
                    return "Error: function not implemented on Windows platform yet. sorry ;(\n";
                }

                case error_t::success:
                {
                    return "function call was successful \n";
                }

                default:
                {
                    return "Error: unspecified Error \n";
                }
            }
        }
        
        errorCategory_t() = default;

        const static errorCategory_t& get()
        {
            const static errorCategory_t category;
            return category;
        }
    };

    inline std::error_code make_error_code(error_t errorCode)
    {
        return std::error_code(static_cast<int>(errorCode), errorCategory_t::get());
    }
};

//ugh I hate this hack
namespace std
{
    template<> struct is_error_code_enum<TinyWindow::error_t> : std::true_type {};
};

namespace TinyWindow
{
    class tWindow
    {
        friend class windowManager;

        using keyEvent_t = std::function<void(tWindow* window, unsigned int key, keyState_t keyState)>;
        using mouseButtonEvent_t = std::function<void(tWindow* window, mouseButton_t mouseButton, buttonState_t buttonState)>;
        using mouseWheelEvent_t = std::function<void(tWindow* window, mouseScroll_t mouseScrollDirection)>;
        using destroyedEvent_t = std::function<void(tWindow* window)>;
        using maximizedEvent_t = std::function<void(tWindow* window)>;
        using minimizedEvent_t = std::function<void(tWindow* window)>;
        using focusEvent_t = std::function<void(tWindow* window, bool isFocused)>;
        using movedEvent_t = std::function<void(tWindow* window, vec2_t<int> windowPosition)>;
        using resizeEvent_t = std::function<void(tWindow* window, vec2_t<unsigned int> windowResolution)>;
        using mouseMoveEvent_t = std::function<void(tWindow* window, vec2_t<int> windowMousePosition, vec2_t<int> screenMousePosition)>;

    public:

        windowSetting_t                         settings;                                           /**< List of User-defined settings for this windowS */
        keyState_t                              keys[last];                                             /**< Record of keys that are either pressed or released in the respective window */
        buttonState_t                           mouseButton[(unsigned int)mouseButton_t::last];         /**< Record of mouse buttons that are either presses or released */
        TinyWindow::vec2_t<int>                 position;                                               /**< Position of the Window relative to the screen co-ordinates */
        TinyWindow::vec2_t<int>                 mousePosition;                                          /**< Position of the Mouse cursor relative to the window co-ordinates */
        TinyWindow::vec2_t<int>                 previousMousePosition;
        bool                                    shouldClose;                                            /**< Whether the Window should be closing */
        bool                                    inFocus;                                                /**< Whether the Window is currently in focus(if it is the current window be used) */
        bool                                    initialized;                                            /**< Whether the window has been successfully initialized */
        bool                                    contextCreated;                                         /**< Whether the OpenGL context has been successfully created */
        bool                                    isCurrentContext;                                       /**< Whether the window is the current window being drawn to */
        unsigned int                            currentStyle;                                           /**< The current style of the window */


        unsigned int                            currentScreenIndex;                                     /**< The Index of the screen currently being rendered to (fullscreen)*/
        bool                                    isFullscreen;                                           /**< Whether the window is currently in fullscreen mode */
        TinyWindow::monitor_t*                  currentMonitor;                                         /**< The monitor that the window is currently rendering to */

    private:

#if defined(TW_USE_VULKAN)
        VkInstance                              vulkanInstanceHandle;
        VkSurfaceKHR                            vulkanSurfaceHandle;
#endif

#if defined(TW_WINDOWS)

        HDC                                     deviceContextHandle;                                    /**< A handle to a device context */
        HGLRC                                   glRenderingContextHandle;                               /**< A handle to an OpenGL rendering context*/
        HPALETTE                                paletteHandle;                                          /**< A handle to a Win32 palette*/
        PIXELFORMATDESCRIPTOR                   pixelFormatDescriptor;                                  /**< Describes the pixel format of a drawing surface*/
        WNDCLASS                                windowClass;                                            /**< Contains the window class attributes */
        HWND                                    windowHandle;                                           /**< A handle to A window */
        HINSTANCE                               instanceHandle;                                         /**< A handle to the window class instance */
        int                                     accumWheelDelta;                                        /**< holds the accumulated mouse wheel delta for this window */

        vec2_t<unsigned int>                    clientArea;                                             /**< the width and height of the client window */

#elif defined(TW_LINUX)

        Window                          windowHandle;                                           /**< The X11 handle to the window. I wish they didn't name the type 'Window' */
        GLXContext                      context;                                                /**< The handle to the GLX rendering context */
        XVisualInfo*                    visualInfo;                                             /**< The handle to the Visual Information. similar purpose to PixelformatDesriptor */
        int*                            attributes;                                             /**< Attributes of the window. RGB, depth, stencil, etc */
        XSetWindowAttributes            setAttributes;                                          /**< The attributes to be set for the window */
        unsigned int                    linuxDecorators;                                        /**< Enabled window decorators */
        Display*                        currentDisplay;                                         /**< Handle to the X11 window */

                                                                        /* these atoms are needed to change window states via the extended window manager*/
        Atom                            AtomState;                      /**< Atom for the state of the window */                            // _NET_WM_STATE
        Atom                            AtomHidden;                     /**< Atom for the current hidden state of the window */             // _NET_WM_STATE_HIDDEN
        Atom                            AtomFullScreen;                 /**< Atom for the full screen state of the window */                // _NET_WM_STATE_FULLSCREEN
        Atom                            AtomMaxHorz;                    /**< Atom for the maximized horizontally state of the window */     // _NET_WM_STATE_MAXIMIZED_HORZ
        Atom                            AtomMaxVert;                    /**< Atom for the maximized vertically state of the window */       // _NET_WM_STATE_MAXIMIZED_VERT
        Atom                            AtomClose;                      /**< Atom for closing the window */                                 // _NET_WM_CLOSE_WINDOW
        Atom                            AtomActive;                     /**< Atom for the active window */                                  // _NET_ACTIVE_WINDOW
        Atom                            AtomDemandsAttention;           /**< Atom for when the window demands attention */                  // _NET_WM_STATE_DEMANDS_ATTENTION
        Atom                            AtomFocused;                    /**< Atom for the focused state of the window */                    // _NET_WM_STATE_FOCUSED
        Atom                            AtomCardinal;                   /**< Atom for cardinal coordinates */                               // _NET_WM_CARDINAL
        Atom                            AtomIcon;                       /**< Atom for the icon of the window */                             // _NET_WM_ICON
        Atom                            AtomHints;                      /**< Atom for the window decorations */                             // _NET_WM_HINTS

        Atom                            AtomWindowType;                 /**< Atom for the type of window */
        Atom                            AtomWindowTypeDesktop;          /**< Atom for the desktop window type */                            //_NET_WM_WINDOW_TYPE_SPLASH
        Atom                            AtomWindowTypeSplash;           /**< Atom for the splash screen window type */
        Atom                            AtomWindowTypeNormal;           /**< Atom for the normal splash screen window type */

        Atom                            AtomAllowedActions;             /**< Atom for allowed window actions */
        Atom                            AtomActionResize;               /**< Atom for allowing the window to be resized */
        Atom                            AtomActionMinimize;             /**< Atom for allowing the window to be minimized */
        Atom                            AtomActionShade;                /**< Atom for allowing the window to be shaded */
        Atom                            AtomActionMaximizeHorz;         /**< Atom for allowing the window to be maximized horizontally */
        Atom                            AtomActionMaximizeVert;         /**< Atom for allowing the window to be maximized vertically */
        Atom                            AtomActionClose;                /**< Atom for allowing the window to be closed */

        Atom                            AtomDesktopGeometry;            /**< Atom for Desktop Geometry */

        enum decorator_t
        {
            linuxBorder = 1L << 1,
            linuxMove = 1L << 2,
            linuxMinimize = 1L << 3,
            linuxMaximize = 1L << 4,
            linuxClose = 1L << 5,
        };

        enum hint_t
        {
            function = 1,
            decorator,
        };

        void InitializeAtoms()
        {
            AtomState = XInternAtom(currentDisplay, "_NET_WM_STATE", false);
            AtomFullScreen = XInternAtom(currentDisplay, "_NET_WM_STATE_FULLSCREEN", false);
            AtomMaxHorz = XInternAtom(currentDisplay, "_NET_WM_STATE_MAXIMIZED_HORZ", false);
            AtomMaxVert = XInternAtom(currentDisplay, "_NET_WM_STATE_MAXIMIZED_VERT", false);
            AtomClose = XInternAtom(currentDisplay, "WM_DELETE_WINDOW", false);
            AtomHidden = XInternAtom(currentDisplay, "_NET_WM_STATE_HIDDEN", false);
            AtomActive = XInternAtom(currentDisplay, "_NET_ACTIVE_WINDOW", false);
            AtomDemandsAttention = XInternAtom(currentDisplay, "_NET_WM_STATE_DEMANDS_ATTENTION", false);
            AtomFocused = XInternAtom(currentDisplay, "_NET_WM_STATE_FOCUSED", false);
            AtomCardinal = XInternAtom(currentDisplay, "CARDINAL", false);
            AtomIcon = XInternAtom(currentDisplay, "_NET_WM_ICON", false);
            AtomHints = XInternAtom(currentDisplay, "_MOTIF_WM_HINTS", true);

            AtomWindowType = XInternAtom(currentDisplay, "_NET_WM_WINDOW_TYPE", false);
            AtomWindowTypeDesktop = XInternAtom(currentDisplay, "_NET_WM_WINDOW_TYPE_UTILITY", false);
            AtomWindowTypeSplash = XInternAtom(currentDisplay, "_NET_WM_WINDOW_TYPE_SPLASH", false);
            AtomWindowTypeNormal = XInternAtom(currentDisplay, "_NET_WM_WINDOW_TYPE_NORMAL", false);

            AtomAllowedActions = XInternAtom(currentDisplay, "_NET_WM_ALLOWED_ACTIONS", false);
            AtomActionResize = XInternAtom(currentDisplay, "WM_ACTION_RESIZE", false);
            AtomActionMinimize = XInternAtom(currentDisplay, "_WM_ACTION_MINIMIZE", false);
            AtomActionShade = XInternAtom(currentDisplay, "WM_ACTION_SHADE", false);
            AtomActionMaximizeHorz = XInternAtom(currentDisplay, "_WM_ACTION_MAXIMIZE_HORZ", false);
            AtomActionMaximizeVert = XInternAtom(currentDisplay, "_WM_ACTION_MAXIMIZE_VERT", false);
            AtomActionClose = XInternAtom(currentDisplay, "_WM_ACTION_CLOSE", false);

            AtomDesktopGeometry = XInternAtom(currentDisplay, "_NET_DESKTOP_GEOMETRY", false);
        }
#endif

    public:

        tWindow(windowSetting_t windowSetting)
        {
            this->settings = windowSetting;

            this->shouldClose = false;

            initialized = false;
            contextCreated = false;
            currentStyle = titleBar | icon | border | minimizeButton | maximizeButton | closeButton | sizeableBorder;

            std::fill(keys, keys + last, keyState_t::up);// = { keyState_t.bad };
            std::fill(mouseButton, mouseButton + (unsigned int)mouseButton_t::last, buttonState_t::up);

            inFocus = false;
            isCurrentContext = false;
            currentScreenIndex = 0;
            isFullscreen = false;
            currentMonitor = nullptr;
            
#if defined(TW_WINDOWS)
            deviceContextHandle = nullptr;
            glRenderingContextHandle = nullptr;
            paletteHandle = nullptr;
            pixelFormatDescriptor = PIXELFORMATDESCRIPTOR();
            windowClass = WNDCLASS();
            windowHandle = nullptr;
            instanceHandle = nullptr;
            accumWheelDelta = 0;
            clientArea = vec2_t<unsigned int>::Zero();
#endif
            
#if defined(__linux__)
            context = 0;
#endif 
        }

        /**
        * Set the Size/Resolution of the given window
        */
        std::error_code SetWindowSize(TinyWindow::vec2_t<unsigned int> newResolution)
        {
            this->settings.resolution = newResolution;
#if defined(TW_WINDOWS)
            SetWindowPos(windowHandle, HWND_TOP,
                position.x, position.y,
                newResolution.x, newResolution.y,
                SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#elif defined(TW_LINUX)
            XResizeWindow(currentDisplay,
                windowHandle, newResolution.x, newResolution.y);
#endif
            return TinyWindow::error_t::success;
        }

        /**
        * Set the Position of the given window relative to screen co-ordinates
        */
        std::error_code SetPosition(TinyWindow::vec2_t<int> newPosition)
        {
            this->position = newPosition;

#if defined(TW_WINDOWS)
            SetWindowPos(windowHandle, HWND_TOP, newPosition.x, newPosition.y,
                settings.resolution.x, settings.resolution.y,
                SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
#elif defined(TW_LINUX)
            XWindowChanges windowChanges;

            windowChanges.x = newPosition.x;
            windowChanges.y = newPosition.y;

            XConfigureWindow(
                currentDisplay,
                windowHandle, CWX | CWY, &windowChanges);
#endif
            return TinyWindow::error_t::success;
        }

        /**
        * Set the mouse Position of the given window's co-ordinates
        */
        std::error_code SetMousePosition(TinyWindow::vec2_t<unsigned int> newMousePosition)
        {
            this->mousePosition.x = newMousePosition.x;
            this->mousePosition.y = newMousePosition.y;
#if defined(TW_WINDOWS)
            POINT mousePoint;
            mousePoint.x = newMousePosition.x;
            mousePoint.y = newMousePosition.y;
            ClientToScreen(windowHandle, &mousePoint);
            SetCursorPos(mousePoint.x, mousePoint.y);
#elif defined(TW_LINUX)
            XWarpPointer(
                currentDisplay,
                windowHandle, windowHandle,
                position.x, position.y,
                resolution.width, resolution.height,
                newMousePosition.x, newMousePosition.y);
#endif
            return TinyWindow::error_t::success;
        }

#if defined(TW_USE_VULKAN) //these are hidden if TW_USE_VULKAN is not defined
        //get reference to instance
        inline VkInstance& GetVulkanInstance()
        {
            return vulkanInstanceHandle;
        }

        //get reference to surface
        inline VkSurfaceKHR& GetVulkanSurface()
        {
            return vulkanSurfaceHandle;
        }
#else
        /**
        * Swap the draw buffers of the given window.
        */
        inline std::error_code SwapDrawBuffers() 
        {
#if defined(TW_WINDOWS)
            SwapBuffers(deviceContextHandle);
#elif defined(TW_LINUX)
            glXSwapBuffers(currentDisplay, windowHandle);
#endif
            return TinyWindow::error_t::success;
        }

        /**
        * Make the given window be the current OpenGL Context to be drawn to
        */
        std::error_code MakeCurrentContext()
        {
#if defined(TW_WINDOWS)
            wglMakeCurrent(deviceContextHandle,
                glRenderingContextHandle);
#elif defined(TW_LINUX)
            glXMakeCurrent(currentDisplay, windowHandle,
                context);
#endif
            return TinyWindow::error_t::success;
        }
#endif
        /**
        * Toggle the minimization state of the given window
        */
        std::error_code Minimize(bool newState)
        {
            if (newState)
            {
                settings.currentState = state_t::minimized;

#if defined(TW_WINDOWS)
                ShowWindow(windowHandle, SW_MINIMIZE);
#elif defined(TW_LINUX)
                XIconifyWindow(currentDisplay,
                    windowHandle, 0);
#endif
            }

            else
            {
                settings.currentState = state_t::normal;
#if defined(TW_WINDOWS)
                ShowWindow(windowHandle, SW_RESTORE);
#elif defined(TW_LINUX)
                XMapWindow(currentDisplay, windowHandle);
#endif
            }
            return TinyWindow::error_t::success;
        }

        /**
        * Toggle the maximization state of the current window
        */
        std::error_code Maximize(bool newState)
        {
            if (newState)
            {
                settings.currentState = state_t::maximized;
#if defined(TW_WINDOWS)
                ShowWindow(windowHandle, SW_MAXIMIZE);
#elif defined(TW_LINUX)
                XEvent currentEvent;
                memset(&currentEvent, 0, sizeof(currentEvent));

                currentEvent.xany.type = ClientMessage;
                currentEvent.xclient.message_type = AtomState;
                currentEvent.xclient.format = 32;
                currentEvent.xclient.window = windowHandle;
                currentEvent.xclient.data.l[0] = (currentState == state_t::maximized);
                currentEvent.xclient.data.l[1] = AtomMaxVert;
                currentEvent.xclient.data.l[2] = AtomMaxHorz;

                XSendEvent(currentDisplay,
                    windowHandle,
                    0, SubstructureNotifyMask, &currentEvent);
#endif
            }

            else
            {
                settings.currentState = state_t::normal;
#if defined(TW_WINDOWS)
                ShowWindow(windowHandle, SW_RESTORE);
#elif defined(TW_LINUX)
                XEvent currentEvent;
                memset(&currentEvent, 0, sizeof(currentEvent));

                currentEvent.xany.type = ClientMessage;
                currentEvent.xclient.message_type = AtomState;
                currentEvent.xclient.format = 32;
                currentEvent.xclient.window = windowHandle;
                currentEvent.xclient.data.l[0] = (currentState == state_t::maximized);
                currentEvent.xclient.data.l[1] = AtomMaxVert;
                currentEvent.xclient.data.l[2] = AtomMaxHorz;

                XSendEvent(currentDisplay, windowHandle,
                    0, SubstructureNotifyMask, &currentEvent);
#endif
            }
            return TinyWindow::error_t::success;
        }

        /**
        * Toggle the given window's full screen mode
        */
        std::error_code SetFullScreen(bool newState /* need to add definition for which screen*/)
        {
            settings.currentState = (newState == true) ? state_t::fullscreen : state_t::normal;

#if defined(TW_WINDOWS)

            SetWindowLongPtr(windowHandle, GWL_STYLE, static_cast<LONG_PTR>(
                WS_SYSMENU | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE));

            RECT desktop;
            GetWindowRect(windowHandle, &desktop);
            MoveWindow(windowHandle, 0, 0, desktop.right, desktop.bottom, true);

#elif defined(TW_LINUX)

            XEvent currentEvent;
            memset(&currentEvent, 0, sizeof(currentEvent));

            currentEvent.xany.type = ClientMessage;
            currentEvent.xclient.message_type = AtomState;
            currentEvent.xclient.format = 32;
            currentEvent.xclient.window = windowHandle;
            currentEvent.xclient.data.l[0] = currentState == state_t::fullscreen;
            currentEvent.xclient.data.l[1] = AtomFullScreen;

            XSendEvent(currentDisplay, windowHandle,
                0, SubstructureNotifyMask, &currentEvent);
#endif
            return TinyWindow::error_t::success;
        }

        /**
        * Toggles full-screen mode for a window by parsing in a monitor and a monitor setting index
        */
        std::error_code ToggleFullscreen(monitor_t* monitor, unsigned int monitorSettingIndex)
        {
#if defined(TW_WINDOWS)
            return Windows_ToggleFullscreen(monitor, monitorSettingIndex);
#elif defined(TW_LINUX)
            return error_t::functionNotImplemented;
#endif
        }

        /**
        * Set the window title bar  by name
        */
        std::error_code SetTitleBar(const char* newTitle)
        {
            if (newTitle != nullptr)
            {
#if defined(TW_WINDOWS)
                SetWindowText(windowHandle, newTitle);
#elif defined(TW_LINUX)
                XStoreName(currentDisplay, windowHandle, newTitle);
#endif
                return TinyWindow::error_t::success;
            }
            return TinyWindow::error_t::invalidTitlebar;
        }

        /**
        * Set the window icon by name (currently not functional)
        */
        std::error_code SetIcon()//const char* windowName, const char* icon, unsigned int width, unsigned int height)
        {
            return TinyWindow::error_t::functionNotImplemented;
        }

        /**
        * Set the window to be in focus by name
        */
        std::error_code Focus(bool newState)
        {
            if (newState)
            {
#if defined(TW_WINDOWS)
                SetFocus(windowHandle);
#elif defined(TW_LINUX)
                XMapWindow(currentDisplay, windowHandle);
#endif
            }

            else
            {
#if defined(_WIN32) || defined(_WIN64)
                SetFocus(nullptr);
#elif defined(TW_LINUX)
                XUnmapWindow(currentDisplay, windowHandle);
#endif
            }
            return TinyWindow::error_t::success;
        }

        /**
        * Restore the window by name
        */
        std::error_code Restore()
        {
#if defined(TW_WINDOWS)
            ShowWindow(windowHandle, SW_RESTORE);
#elif defined(TW_LINUX)
            XMapWindow(currentDisplay, windowHandle);
#endif
            return TinyWindow::error_t::success;
        }

        /**
        * Set the window style preset by name
        */
        std::error_code SetStyle(style_t windowStyle)
        {
#if defined(TW_WINDOWS)
            switch (windowStyle)
            {
            case style_t::normal:
            {
                EnableDecorators(titleBar | border |
                    closeButton | minimizeButton | maximizeButton | sizeableBorder);
                break;
            }

            case style_t::popup:
            {
                EnableDecorators(0);
                break;
            }

            case style_t::bare:
            {
                EnableDecorators(titleBar | border);
                break;
            }

            default:
            {
                return TinyWindow::error_t::invalidWindowStyle;
            }
            }

#elif defined(TW_LINUX)
            switch (windowStyle)
            {
            case style_t::normal:
            {
                linuxDecorators = (1L << 2);
                currentStyle = linuxMove | linuxClose |
                    linuxMaximize | linuxMinimize;
                long Hints[5] = { hint_t::function | hint_t::decorator, currentStyle, linuxDecorators, 0, 0 };

                XChangeProperty(currentDisplay, windowHandle, AtomHints, XA_ATOM, 32, PropModeReplace,
                    (unsigned char*)Hints, 5);

                XMapWindow(currentDisplay, windowHandle);
                break;
            }

            case style_t::bare:
            {
                linuxDecorators = (1L << 2);
                currentStyle = (1L << 2);
                long Hints[5] = { function | decorator, currentStyle, linuxDecorators, 0, 0 };

                XChangeProperty(currentDisplay, windowHandle, AtomHints, XA_ATOM, 32, PropModeReplace,
                    (unsigned char*)Hints, 5);

                XMapWindow(currentDisplay, windowHandle);
                break;
            }

            case style_t::popup:
            {
                linuxDecorators = 0;
                currentStyle = (1L << 2);
                long Hints[5] = { function | decorator, currentStyle, linuxDecorators, 0, 0 };

                XChangeProperty(currentDisplay, windowHandle, AtomHints, XA_ATOM, 32, PropModeReplace,
                    (unsigned char*)Hints, 5);

                XMapWindow(currentDisplay, windowHandle);
                break;
            }

            default:
            {
                return TinyWindow::error_t::invalidWindowStyle;
            }
            }
#endif
            return TinyWindow::error_t::success;
        }

        /**
        * Enable window decorators by name
        */
        std::error_code EnableDecorators(unsigned int decorators)
        {
#if defined(TW_WINDOWS)

            currentStyle = WS_VISIBLE | WS_CLIPSIBLINGS;

            if (decorators & border)
            {
                currentStyle |= WS_BORDER;
            }

            if (decorators & titleBar)
            {
                currentStyle |= WS_CAPTION;
            }

            if (decorators & icon)
            {
                currentStyle |= WS_ICONIC;
            }

            if (decorators & closeButton)
            {
                currentStyle |= WS_SYSMENU;
            }

            if (decorators & minimizeButton)
            {
                currentStyle |= WS_MINIMIZEBOX | WS_SYSMENU;
            }

            if (decorators & maximizeButton)
            {
                currentStyle |= WS_MAXIMIZEBOX | WS_SYSMENU;
            }

            if (decorators & sizeableBorder)
            {
                currentStyle |= WS_SIZEBOX;
            }

            SetWindowLongPtr(windowHandle, GWL_STYLE, static_cast<LONG_PTR>(currentStyle));
            SetWindowPos(windowHandle, HWND_TOP, position.x, position.y,
                settings.resolution.width, settings.resolution.height, SWP_FRAMECHANGED);

#elif defined(TW_LINUX)

            if (decorators & closeButton)
            {
                currentStyle |= linuxClose;
                decorators = 1;
            }

            if (decorators & minimizeButton)
            {
                currentStyle |= linuxMinimize;
                decorators = 1;
            }

            if (decorators & maximizeButton)
            {
                currentStyle |= linuxMaximize;
                decorators = 1;
            }

            if (decorators & icon)
            {
                //Linux (at least cinnamon) does not have icons in the window. only in the task bar icon
            }

            //just need to set it to 1 to enable all decorators that include title bar 
            if (decorators & titleBar)
            {
                decorators = 1;
            }

            if (decorators & border)
            {
                decorators = 1;
            }

            if (decorators & sizeableBorder)
            {
                decorators = 1;
            }

            long hints[5] = { function | decorator, currentStyle, decorators, 0, 0 };

            XChangeProperty(currentDisplay, windowHandle, AtomHints, XA_ATOM, 32,
                PropModeReplace, (unsigned char*)hints, 5);

            XMapWindow(currentDisplay, windowHandle);
#endif
            return TinyWindow::error_t::success;
        }

        /**
        * Disable windows decorators by name
        */
        std::error_code DisableDecorators(unsigned int decorators)
        {
#if defined(TW_WINDOWS)
            if (decorators & border)
            {
                currentStyle &= ~WS_BORDER;
            }

            if (decorators & titleBar)
            {
                currentStyle &= ~WS_CAPTION;
            }

            if (decorators & icon)
            {
                currentStyle &= ~WS_ICONIC;
            }

            if (decorators & closeButton)
            {
                currentStyle &= ~WS_SYSMENU;
            }

            if (decorators & minimizeButton)
            {
                currentStyle &= ~WS_MINIMIZEBOX;
            }

            if (decorators & maximizeButton)
            {
                currentStyle &= ~WS_MAXIMIZEBOX;
            }

            if (decorators & sizeableBorder)
            {
                currentStyle &= ~WS_THICKFRAME;
            }

            SetWindowLongPtr(windowHandle, GWL_STYLE,
                static_cast<LONG_PTR>(currentStyle | WS_VISIBLE));

            SetWindowPos(windowHandle, HWND_TOPMOST, position.x, position.y,
                settings.resolution.width, settings.resolution.height, SWP_FRAMECHANGED);
#elif defined(TW_LINUX)
            if (decorators & closeButton)
            {
                //I hate doing this but it is necessary to keep functionality going.
                bool minimizeEnabled = false;
                bool maximizeEnabled = false;

                if (decorators & maximizeButton)
                {
                    maximizeEnabled = true;
                }

                if (decorators & minimizeButton)
                {
                    minimizeEnabled = true;
                }

                currentStyle &= ~linuxClose;

                if (maximizeEnabled)
                {
                    currentStyle |= linuxMaximize;
                }

                if (minimizeEnabled)
                {
                    currentStyle |= linuxMinimize;
                }

                decorators = 1;
            }

            if (decorators & minimizeButton)
            {
                currentStyle &= ~linuxMinimize;
                decorators = 1;
            }

            if (decorators & maximizeButton)
            {
                bool minimizeEnabled = false;

                if (decorators & minimizeButton)
                {
                    minimizeEnabled = true;
                }

                currentStyle &= ~linuxMaximize;

                if (minimizeEnabled)
                {
                    currentStyle |= linuxMinimize;
                }

                decorators = 1;
            }

            if (decorators & icon)
            {
                //Linux (at least cinnamon) does not have icons in the window. only in the taskb ar icon
            }

            //just need to set it to 1 to enable all decorators that include title bar 
            if (decorators & titleBar)
            {
                decorators = linuxBorder;
            }

            if (decorators & border)
            {
                decorators = 0;
            }

            if (decorators & sizeableBorder)
            {
                decorators = 0;
            }

            long hints[5] = { function | decorator, currentStyle, decorators, 0, 0 };

            XChangeProperty(currentDisplay, windowHandle, AtomHints, XA_ATOM, 32,
                PropModeReplace, (unsigned char*)hints, 5);

            XMapWindow(currentDisplay, windowHandle);
#endif
            return TinyWindow::error_t::success;
        }

        //if windows is defined then allow the user to only GET the necessary info
#if defined(TW_WINDOWS)

        inline HDC GetDeviceContextDeviceHandle()
        {
            return deviceContextHandle;
        }

        inline HGLRC GetGLRenderingContextHandle()
        {
            return glRenderingContextHandle;
        }

        inline HWND GetWindowHandle()
        {
            return windowHandle;
        }

        inline HINSTANCE GetWindowClassInstance()
        {
            return instanceHandle;
        }

        std::error_code Windows_ToggleFullscreen(monitor_t* monitor, unsigned int monitorSettingIndex)
        {
            currentMonitor = monitor;

            DEVMODE devMode;
            ZeroMemory(&devMode, sizeof(DEVMODE));
            devMode.dmSize = sizeof(DEVMODE);
            int err = 0;
            if (isFullscreen)
            {
                err = ChangeDisplaySettingsEx(currentMonitor->displayName.c_str(), nullptr, nullptr, CDS_FULLSCREEN, nullptr);
            }

            else
            {
                if (monitorSettingIndex < (monitor->settings.size() - 1))
                {
                    monitorSetting_t* selectedSetting = monitor->settings[monitorSettingIndex];
                    devMode.dmPelsWidth = selectedSetting->resolution.width;
                    devMode.dmPelsHeight = selectedSetting->resolution.height;
                    devMode.dmBitsPerPel = settings.colorBits * 4;
                    devMode.dmDisplayFrequency = selectedSetting->displayFrequency;
                    devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
                    err = ChangeDisplaySettingsEx(currentMonitor->displayName.c_str(), &devMode, nullptr, CDS_FULLSCREEN, nullptr);
                }
                else
                {
                    return error_t::invalidMonitorSettingIndex;
                }
            }

            switch (err)
            {
            case DISP_CHANGE_SUCCESSFUL:
            {
                isFullscreen = !isFullscreen;
                if (isFullscreen)
                {
                    SetStyle(style_t::popup);
                }

                else
                {
                    SetStyle(style_t::normal);
                }

                break;
            }

            case DISP_CHANGE_BADDUALVIEW:
            {
                return error_t::windowsFullscreenBadDualView;
            }
            case DISP_CHANGE_BADFLAGS:
            {
                return error_t::windowsFullscreenBadFlags;
            }
            case DISP_CHANGE_BADMODE:
            {
                return error_t::windowsFullscreenBadMode;
            }
            case DISP_CHANGE_BADPARAM:
            {
                return error_t::WindowsFullscreenBadParam;
            }
            case DISP_CHANGE_FAILED:
            {
                return error_t::WindowsFullscreenChangeFailed;
            }
            case DISP_CHANGE_NOTUPDATED:
            {
                return error_t::WindowsFullscreenNotUpdated;
            }
            }

            SetPosition(vec2_t<int>((int)monitor->extents.left, (int)monitor->extents.top));
            return error_t::success;
        }

#elif defined(TW_LINUX)

        Window GetWindowHandle()
        {
            return windowHandle;
        }

        GLXContext GetGLXContext()
        {
            return context;
        }

        Display* GetCurrentDisplay()
        {
            return currentDisplay;
        }
#endif
    };

    class windowManager 
    {
    public:
        keyEvent_t                              keyEvent;                                               /**< This is the callback to be used when a key has been pressed */
        mouseButtonEvent_t                      mouseButtonEvent;                                       /**< This is the callback to be used when a mouse button has been pressed */
        mouseWheelEvent_t                       mouseWheelEvent;                                        /**< This is the callback to be used when the mouse wheel has been scrolled. */
        destroyedEvent_t                        destroyedEvent;                                         /**< This is the callback to be used when the window has been closed in a non-programmatic fashion */
        maximizedEvent_t                        maximizedEvent;                                         /**< This is the callback to be used when the window has been maximized in a non-programmatic fashion */
        minimizedEvent_t                        minimizedEvent;                                         /**< This is the callback to be used when the window has been minimized in a non-programmatic fashion */
        focusEvent_t                            focusEvent;                                             /**< This is the callback to be used when the window has been given focus in a non-programmatic fashion */
        movedEvent_t                            movedEvent;                                             /**< This is the callback to be used the window has been moved in a non-programmatic fashion */
        resizeEvent_t                           resizeEvent;                                            /**< This is a callback to be used when the window has been resized in a non-programmatic fashion */
        mouseMoveEvent_t                        mouseMoveEvent;                                         /**< This is a callback to be used when the mouse has been moved */
        fileDropEvent_t                         fileDropEvent;                                          /**< This is a callback to be used when files have been dragged onto a window */

        windowManager(/*error_t* errorCode = nullptr*/)
        {
            /*if (errorCode != nullptr)
            {
                *errorCode = error_t::success;
            }*/
    #if defined(TW_WINDOWS)
            HWND desktopHandle = GetDesktopWindow();

            if (desktopHandle)
            {
                bestPixelFormat = nullptr;
                GetScreenInfo();
                CreateDummyContext();
                if (InitExtensions() == error_t::success)
                {
                    //delete the dummy context and make the current context null
                    wglMakeCurrent(dummyDeviceContextHandle, nullptr);
                    wglMakeCurrent(dummyDeviceContextHandle, nullptr);
                    wglDeleteContext(dummyGLContextHandle);
                    ShutdownDummy();
                }

                else
                {
                    //dummy context has failed so you must use older WGL/openGL methods
                }

                gamepadList.resize(4, nullptr);
                Windows_InitGamepad();
            }
    #elif defined(TW_LINUX)
            currentDisplay = XOpenDisplay(0);

            if (!currentDisplay)
            {
                return;
            }

            /*screenResolution.x = WidthOfScreen(
                XScreenOfDisplay(currentDisplay,
                    DefaultScreen(currentDisplay)));

            screenResolution.y = HeightOfScreen(
                XScreenOfDisplay(currentDisplay,
                    DefaultScreen(currentDisplay)));*/
    #endif


        }

        /**
         * Shutdown and delete all windows in the manager
         */
        ~windowManager()
        {
            assert(windowList.empty());
            ShutDown();
        }

        /**
         * Use this to shutdown the window manager when your program is finished
         */
         void ShutDown()
         {
            //windowList.empty();
    #if defined(TW_WINDOWS)
             ResetMonitors();
    #elif defined(TW_LINUX)
            Linux_Shutdown();
    #endif
            
            for (auto & windowIndex : windowList)
            {
                ShutdownWindow(windowIndex.get());
            }
            windowList.clear();
        }

        /**
         * Use this to add a window to the manager. returns a pointer to the manager which allows for the easy creation of multiple windows
         */
        tWindow* AddWindow(windowSetting_t windowSetting)
        {
            if (windowSetting.name != nullptr)
            {
                std::unique_ptr<tWindow> newWindow(new tWindow(windowSetting));
                windowList.push_back(std::move(newWindow));
                InitializeWindow(windowList.back().get());

                return windowList.back().get();
            }
            return nullptr;
        }

        tWindow* AddSharedWindow(tWindow* sourceWindow, windowSetting_t windowSetting)
        {
            if (windowSetting.name != nullptr)
            {
                std::unique_ptr<tWindow> newWindow(new tWindow(windowSetting));
                windowList.push_back(std::move(newWindow));
                InitializeWindow(windowList.back().get());
                ShareContexts(sourceWindow, windowList.back().get());

                return windowList.back().get();
            }
            return nullptr;
        }

        /**
         * Return the total amount of windows the manager has
         */
        int GetNumWindows()
        {
            return (int)windowList.size();
        }

        /**
        * Return the mouse position in screen co-ordinates
        */
        TinyWindow::vec2_t<int> GetMousePositionInScreen()
        {
            return screenMousePosition;
        }

        /**
         * Set the position of the mouse cursor relative to screen co-ordinates
         */
        void SetMousePositionInScreen(TinyWindow::vec2_t<int> mousePosition)
        {
            screenMousePosition.x = mousePosition.x;
            screenMousePosition.y = mousePosition.y;

    #if defined(TW_WINDOWS)
            SetCursorPos(screenMousePosition.x, screenMousePosition.y);
    #elif defined(TW_LINUX)
            /*XWarpPointer(currentDisplay, None,
                XDefaultRootWindow(currentDisplay), 0, 0,
                screenResolution.x,
                screenResolution.y,
                screenMousePosition.x, screenMousePosition.y);*/
    #endif
        }

        /**
        * Ask the window manager to poll for events
        */
        inline void PollForEvents()
        {
    #if defined(TW_WINDOWS)
            //only process events if there are any to process
            while (PeekMessage(&winMessage, nullptr, 0, 0, PM_REMOVE))
            {
                //the only place I can see this being needed if someone called PostQuitMessage manually
                TranslateMessage(&winMessage);
                DispatchMessage(&winMessage);
                if (winMessage.message == WM_QUIT)
                {
                    ShutDown();
                }
            }
            
    #elif defined(TW_LINUX)
            //if there are any events to process
            if (XEventsQueued(currentDisplay, QueuedAfterReading))
            {
                XNextEvent(currentDisplay, &currentEvent);
                Linux_ProcessEvents(currentEvent);
            }
    #endif

#if !defined(TW_NO_GAMEPAD_POLL)
            PollGamepads();
#endif
        }

        /**
        * Ask the window manager to wait for events
        */
        inline void WaitForEvents()
        {
    #if defined(TW_WINDOWS)
            //process even if there aren't any to process
            GetMessage(&winMessage, nullptr, 0, 0);
            TranslateMessage(&winMessage);
            DispatchMessage(&winMessage);
            if (winMessage.message == WM_QUIT)
            {
                ShutDown();
                return;
            }
    #elif defined(TW_LINUX)
            //even if there aren't any events to process
            XNextEvent(currentDisplay, &currentEvent);
            Linux_ProcessEvents(currentEvent);
    #endif
        }

        /**
        * Remove window from the manager by name
        */
        std::error_code RemoveWindow(tWindow* window)
        {
            if (window != nullptr)
            {
                ShutdownWindow(window);
                return TinyWindow::error_t::success;
            }
            return TinyWindow::error_t::windowInvalid;
        }

        /**
        * Set window swap interval
        */
        std::error_code SetWindowSwapInterval(tWindow* window, int interval)
        {
#if defined(TW_WINDOWS)
            if (swapControlEXT && wglSwapIntervalEXT != nullptr)
            {
                HGLRC previousGLContext = wglGetCurrentContext();
                HDC previousDeviceContext = wglGetCurrentDC();
                wglMakeCurrent(window->deviceContextHandle, window->glRenderingContextHandle);
                wglSwapIntervalEXT(interval);
                wglMakeCurrent(previousDeviceContext, previousGLContext);
            }
            return error_t::success;
#elif defined(TW_LINUX)
            
#endif
        }
        
        /**
        * get the swap interval (V-Sync) of the given window
        */
        int GetWindowSwapInterval(tWindow* window)
        {
#if defined(TW_WINDOWS)
            
            if (wglGetSwapIntervalEXT && swapControlEXT)
            {
                HGLRC previousGLContext = wglGetCurrentContext();
                HDC previousDeviceContext = wglGetCurrentDC();
                wglMakeCurrent(window->deviceContextHandle, window->glRenderingContextHandle);
                int interval = wglGetSwapIntervalEXT();
                wglMakeCurrent(previousDeviceContext, previousGLContext);
                return interval;
            }
#elif defined(TW_LINUX)

#endif
            else
            {
                return 0;
            }

        }

        /**
        *   get the list of monitors connected to the system
        */
        std::vector<monitor_t*> GetMonitors()
        {
            return monitorList;
        }

        /**
        *   get the list of gamepad states connected to the system
        */
        std::vector<gamepad_t*> GetGamepads()
        {
            return gamepadList;
        }

    private:

        std::vector<std::unique_ptr<tWindow>>       windowList;
        std::vector<monitor_t*>                     monitorList;
        std::vector<formatSetting_t*>               formatList;
        std::vector<gamepad_t*>                     gamepadList;

        TinyWindow::vec2_t<int>                     screenMousePosition;

        std::error_code CreateDummyContext()
        {
#if defined(TW_WINDOWS)
            return Windows_CreateDummyContext();
#elif defined(TW_LINUX)

#endif // 
        }

        std::error_code InitExtensions()
        {
#if defined(TW_WINDOWS)
            return Windows_InitExtensions();            
#elif defined(TW_LINUX)
            return error_t::linuxFunctionNotImplemented;
#endif
        }

        void InitializeWindow(tWindow* window)
        {
    #if defined(TW_WINDOWS)
            Windows_InitializeWindow(window);
    #elif defined(TW_LINUX)
            Linux_InitializeWindow(window);
    #endif
        }

        std::error_code InitializeGL(tWindow* window)
        {
#if defined(TW_WINDOWS)
            return Windows_InitGL(window);
#elif defined(TW_LINUX)
            return Linux_InitGL(window);
    #endif
        }

        void GetScreenInfo()
        {
#if defined(TW_WINDOWS)
            Windows_GetScreenInfo();
#elif defined(TW_LINUX)

#endif
        }

        void CheckWindowScreen(tWindow* window)
        {
#if defined(TW_WINDOWS)
            //for each monitor
            for (auto & monitorIndex : monitorList)
            {
                if (monitorIndex->monitorHandle == MonitorFromWindow(window->windowHandle, MONITOR_DEFAULTTONEAREST))
                {
                    window->currentMonitor = monitorIndex;
                }
            }
#endif
        }

        void ShutdownWindow(tWindow* window)
        {
            if (destroyedEvent != nullptr)
            {
                destroyedEvent(window);
            }
    #if defined(TW_WINDOWS)
            window->shouldClose = true;
            if (window->glRenderingContextHandle)
            {
                wglMakeCurrent(nullptr, nullptr);
                wglDeleteContext(window->glRenderingContextHandle);
            }

            if (window->paletteHandle)
            {
                DeleteObject(window->paletteHandle);
            }
            ReleaseDC(window->windowHandle, window->deviceContextHandle);
            UnregisterClass(window->settings.name, window->instanceHandle);

            FreeModule(window->instanceHandle);

            window->deviceContextHandle = nullptr;
            window->windowHandle = nullptr;
            window->glRenderingContextHandle = nullptr;

    #elif defined(TW_LINUX)
            if (window->currentState == state_t::fullscreen)
            {
                window->Restore();
            }

            glXDestroyContext(currentDisplay, window->context);
            XUnmapWindow(currentDisplay, window->windowHandle);
            XDestroyWindow(currentDisplay, window->windowHandle);
            window->windowHandle = 0;
            window->context = 0;
    #endif

            for (auto it = windowList.begin(); it != windowList.end(); ++it)
            {
                if (window == it->get())
                {
                    it->release();
                    windowList.erase(it);
                    break;
                }
            }
        }

        void ShareContexts(tWindow* sourceWindow, tWindow* newWindow)
        {
#if defined(TW_WINDOWS)
            Windows_ShareContexts(sourceWindow, newWindow);
#elif defined(TW_LINUX)
            //TODO: need to implement shared context functionality
#endif
        }

        void ResetMonitors()
        {
#if defined(TW_WINDOWS)
            Windows_ResetMonitors();
#elif defined(TW_LINUX)

#endif
        }

        void PollGamepads()
        {
#if defined(TW_WINDOWS)
            Windows_PollGamepads();
#elif defined(TW_LINUX)

#endif
        }
    
#if defined(TW_WINDOWS)

        MSG                                         winMessage;
        HWND                                        dummyWindowHandle;
        HGLRC                                       dummyGLContextHandle;           /**< A handle to the dummy OpenGL rendering context*/
        HDC                                         dummyDeviceContextHandle;
        
        HINSTANCE                                   dummyWindowInstance;
        //wgl extensions
        PFNWGLGETEXTENSIONSSTRINGARBPROC            wglGetExtensionsStringARB;
        PFNWGLGETEXTENSIONSSTRINGEXTPROC            wglGetExtensionsStringEXT;
        PFNWGLCHOOSEPIXELFORMATARBPROC              wglChoosePixelFormatARB;
        PFNWGLCHOOSEPIXELFORMATEXTPROC              wglChoosePixelFormatEXT;
        PFNWGLCREATECONTEXTATTRIBSARBPROC           wglCreateContextAttribsARB;
        PFNWGLSWAPINTERVALEXTPROC                   wglSwapIntervalEXT;
        PFNWGLGETSWAPINTERVALEXTPROC                wglGetSwapIntervalEXT;
        PFNWGLGETPIXELFORMATATTRIBFVARBPROC         wglGetPixelFormatAttribfvARB;
        PFNWGLGETPIXELFORMATATTRIBFVEXTPROC         wglGetPixelFormatAttribfvEXT;
        PFNWGLGETPIXELFORMATATTRIBIVARBPROC         wglGetPixelFormatAttribivARB;
        PFNWGLGETPIXELFORMATATTRIBIVEXTPROC         wglGetPixelFormatAttribivEXT;

        bool                                        swapControlEXT;
        bool                                        wglFramebufferSRGBCapableARB;

        formatSetting_t*                            bestPixelFormat;

        //the window procedure for all windows. This is used mainly to handle window events
        static LRESULT CALLBACK WindowProcedure(HWND windowHandle, unsigned int winMessage, WPARAM wordParam, LPARAM longParam)
        {
            windowManager* manager = (windowManager*)GetWindowLongPtr(windowHandle, GWLP_USERDATA);
            tWindow* window = nullptr;
            if (manager != nullptr)
            {
                window = manager->GetWindowByHandle(windowHandle);
            }

            unsigned int translatedKey = 0;
            static bool wasLowerCase = false;
        
            switch (winMessage)
            {
                case WM_DESTROY:
                {
                    if (manager != nullptr)
                    {
                        window->shouldClose = true;

                        if (manager->destroyedEvent != nullptr)
                        {
                            manager->destroyedEvent(window);
                        }
                        //don't shutdown automatically, let people choose when to unload
                        //manager->ShutdownWindow(window);
                    }
                    break;
                }

                case WM_MOVE:
                {
                    window->position.x = LOWORD(longParam);
                    window->position.y = HIWORD(longParam);
                    manager->CheckWindowScreen(window);

                    if (manager->movedEvent != nullptr)
                    {
                        manager->movedEvent(window, window->position);
                    }

                    break;
                }

                case WM_MOVING:
                {
                    window->position.x = LOWORD(longParam);
                    window->position.y = HIWORD(longParam);

                    if (manager->movedEvent != nullptr)
                    {
                        manager->movedEvent(window, window->position);
                    }
                    break;
                }

                case WM_SIZE:
                {
                    //high and low word are the client resolution. will need to change this
                    window->settings.resolution.width = (unsigned int)LOWORD(longParam);
                    window->settings.resolution.height = (unsigned int)HIWORD(longParam);

                    RECT tempRect;
                    GetClientRect(window->windowHandle, &tempRect);
                    window->clientArea.width = tempRect.right;
                    window->clientArea.height = tempRect.bottom;

                    GetWindowRect(window->windowHandle, &tempRect);
                    //window->resolution.width = tempRect.right;
                    //window->resolution.height = tempRect.bottom;

                    switch (wordParam)
                    {
                        case SIZE_MAXIMIZED:
                        {
                            if (manager->maximizedEvent != nullptr)
                            {
                                manager->maximizedEvent(window);
                            }

                            break;
                        }

                        case SIZE_MINIMIZED:
                        {
                            if (manager->minimizedEvent != nullptr)
                            {
                                manager->minimizedEvent(window);
                            }
                            break;
                        }

                        default:
                        {
                            if (manager->resizeEvent != nullptr)
                            {
                                manager->resizeEvent(window, window->settings.resolution);
                            }
                            break;
                        }
                    }
                    break;
                }
                //only occurs when the window size is being dragged
                case WM_SIZING:
                {
                    RECT tempRect;
                    GetWindowRect(window->windowHandle, &tempRect);
                    window->settings.resolution.width = tempRect.right;
                    window->settings.resolution.height = tempRect.bottom;

                    GetClientRect(window->windowHandle, &tempRect);
                    window->clientArea.width = tempRect.right;
                    window->clientArea.height = tempRect.bottom;

                    if (manager->resizeEvent != nullptr)
                    {
                        manager->resizeEvent(window, window->settings.resolution);
                    }

                    UpdateWindow(window->windowHandle);// , NULL, true);
                    break;
                }

                case WM_INPUT:
                {
                    char buffer[sizeof(RAWINPUT)] = {};
                    UINT size = sizeof(RAWINPUT);
                    GetRawInputData(reinterpret_cast<HRAWINPUT>(longParam), RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER));

                    RAWINPUT* rawInput = reinterpret_cast<RAWINPUT*>(buffer);
                    switch (rawInput->header.dwType)
                    {
                        //grab raw keyboard info
                        case RIM_TYPEKEYBOARD:
                        {
                            const RAWKEYBOARD& rawKB = rawInput->data.keyboard;
                            unsigned int virtualKey = rawKB.VKey;
                            unsigned int scanCode = rawKB.MakeCode;
                            unsigned int flags = rawKB.Flags;
                            bool isE0 = false;
                            bool isE1 = false;

                            if (virtualKey == 255)
                            {
                                break;
                            }

                            keyState_t keyState;
                            if ((flags & RI_KEY_BREAK) != 0)
                            {
                                keyState = keyState_t::up;
                            }

                            else
                            {
                                keyState = keyState_t::down;
                            }

                            if ((flags & RI_KEY_E0))
                            {
                                isE0 = true;
                            }

                            if ((flags & RI_KEY_E1))
                            {
                                isE1 = true;
                            }

                            if (virtualKey == VK_SHIFT)
                            {
                                virtualKey = MapVirtualKey(scanCode, MAPVK_VSC_TO_VK_EX);

                                if (virtualKey == VK_LSHIFT)
                                {
                                    window->keys[leftShift] = keyState;
                                }

                                else if (virtualKey == VK_RSHIFT)
                                {
                                    window->keys[rightShift] = keyState;
                                }
                            }

                            else if (virtualKey == VK_NUMLOCK)
                            {
                                //in raw input there is a big problem with PAUSE/break and numlock
                                //the scancode needs to be remapped and have the extended bit set
                                scanCode = (MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC) | 0x100);

                                if (scanCode == VK_PAUSE)
                                {
                                }

                                //std::bitset<64> bits(scanCode);
                                //bits.set(24);
                            }

                            if (isE1)
                            {
                                if (virtualKey == VK_PAUSE)
                                {
                                    scanCode = 0x45; //the E key???
                                }

                                else
                                {
                                    scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);
                                }
                            }

                            translatedKey = 0;

                            switch (virtualKey)
                            {
                                case VK_CONTROL:
                                {
                                    translatedKey = (isE0) ? rightControl : leftControl;
                                    break;
                                }
                            }
                        }

                        //grab mouse info
                        case RIM_TYPEMOUSE:
                        {
                            break;
                        }

                        //grab joystick info
                        case RIM_TYPEHID:
                        {
                            break;
                        }
                    }
                }
                
                case WM_CHAR:
                {
                    //WM_KEYUP/DOWN cannot tell between uppercase and lowercase since it takes directly from the keyboard
                    //so WM_CHAR is needed to determine casing. still a pain though to see whether the key
                    //was pressed or released.
                    wasLowerCase = islower(static_cast<int>(wordParam)) != 0;
                    window->keys[wordParam] = keyState_t::down;
                    if (manager->keyEvent != nullptr)
                    {
                        manager->keyEvent(window, static_cast<int>(wordParam), keyState_t::down);
                    }
                    break;
                }

                case WM_KEYDOWN:
                {
                    switch (DetermineLeftOrRight(wordParam, longParam))
                    {
                        case VK_LCONTROL:
                        {
                            window->keys[leftControl] = keyState_t::down;
                            translatedKey = leftControl;
                            break;
                        }

                        case VK_RCONTROL:
                        {
                            window->keys[rightControl] = keyState_t::down;
                            translatedKey = rightControl;
                            break;
                        }

                        case VK_LSHIFT:
                        {
                            window->keys[leftShift] = keyState_t::down;
                            translatedKey = leftShift;
                            break;
                        }

                        case VK_RSHIFT:
                        {
                            window->keys[rightShift] = keyState_t::down;
                            translatedKey = rightShift;
                            break;
                        }
                    
                        default:
                        {
                            translatedKey = Windows_TranslateKey(wordParam);
                            if (translatedKey != 0)
                            {
                                window->keys[translatedKey] = keyState_t::down;
                            }
                            break;
                        }
                    }

                    if (manager->keyEvent != nullptr && translatedKey != 0)
                    {
                        manager->keyEvent(window, translatedKey, keyState_t::down);
                    }
                    break;
                }

                case WM_KEYUP:
                {
                    switch (DetermineLeftOrRight(wordParam, longParam))
                    {
                        case VK_LCONTROL:
                        {
                            window->keys[leftControl] = keyState_t::up;
                            translatedKey = leftControl;
                            break;
                        }

                        case VK_RCONTROL:
                        {
                            window->keys[rightControl] = keyState_t::up;
                            translatedKey = rightControl;
                            break;
                        }

                        case VK_LSHIFT:
                        {
                            window->keys[leftShift] = keyState_t::up;
                            translatedKey = leftShift;
                            break;
                        }

                        case VK_RSHIFT:
                        {
                            window->keys[rightShift] = keyState_t::up;
                            translatedKey = rightShift;
                            break;
                        }

                        default:
                        {
                            translatedKey = Windows_TranslateKey(wordParam);
                            if (translatedKey != 0)
                            {
                                window->keys[translatedKey] = keyState_t::up;
                            }

                            else
                            {
                                //if it was lowercase 
                                if (wasLowerCase)
                                {
                                    //change the wordParam to lowercase
                                    translatedKey = tolower(static_cast<unsigned int>(wordParam));
                                }
                                else
                                {
                                    //keep it as is if it isn't
                                    translatedKey = static_cast<unsigned int>(wordParam);
                                }

                                window->keys[translatedKey] = keyState_t::up;
                            }
                            break;
                        }
                    }

                    if (manager->keyEvent != nullptr)
                    {
                        manager->keyEvent(window, translatedKey, keyState_t::up);
                    }
                    break;
                }

                case WM_SYSKEYDOWN:
                {
                    translatedKey = 0;

                    switch (DetermineLeftOrRight(wordParam, longParam))
                    {
                        case VK_LMENU:
                        {
                            window->keys[leftAlt] = keyState_t::down;
                            translatedKey = leftAlt;
                            break;
                        }

                        case VK_RMENU:
                        {
                            window->keys[rightAlt] = keyState_t::down;
                            translatedKey = rightAlt;
                            break;
                        }
                    }

                    if (manager->keyEvent != nullptr)
                    {
                        manager->keyEvent(window, translatedKey, keyState_t::down);
                    }

                    break;
                }

                case WM_SYSKEYUP:
                {
                    translatedKey = 0;
                    switch (DetermineLeftOrRight(wordParam, longParam))
                    {
                        case VK_LMENU:
                        {
                            window->keys[leftAlt] = keyState_t::up;
                            translatedKey = leftAlt;
                            break;
                        }


                        case VK_RMENU:
                        {
                            window->keys[rightAlt] = keyState_t::up;
                            translatedKey = rightAlt;
                            break;
                        }

                        default:
                        {
                            break;
                        }
                    }

                    if (manager->keyEvent != nullptr)
                    {
                        manager->keyEvent(window, translatedKey, keyState_t::up);
                    }
                    break;
                }

                case WM_MOUSEMOVE:
                {
                    window->previousMousePosition = window->mousePosition;
                    window->mousePosition.x = (int)LOWORD(longParam);
                    window->mousePosition.y = (int)HIWORD(longParam);

                    POINT point;
                    point.x = (LONG)window->mousePosition.x;
                    point.y = (LONG)window->mousePosition.y;

                    ClientToScreen(windowHandle, &point);

                    if (manager->mouseMoveEvent != nullptr)
                    {
                        manager->mouseMoveEvent(window, window->mousePosition, vec2_t<int>(point.x, point.y));
                    }
                    break;
                }

                case WM_LBUTTONDOWN:
                {
                    window->mouseButton[(unsigned int)mouseButton_t::left] = buttonState_t::down;

                    if (manager->mouseButtonEvent != nullptr)
                    {
                        manager->mouseButtonEvent(window, mouseButton_t::left, buttonState_t::down);
                    }
                    break;
                }

                case WM_LBUTTONUP:
                {
                    window->mouseButton[(unsigned int)mouseButton_t::left] = buttonState_t::up;

                    if (manager->mouseButtonEvent != nullptr)
                    {
                        manager->mouseButtonEvent(window, mouseButton_t::left, buttonState_t::up);
                    }
                    break;
                }

                case WM_RBUTTONDOWN:
                {
                    window->mouseButton[(unsigned int)mouseButton_t::right] = buttonState_t::down;

                    if (manager->mouseButtonEvent != nullptr)
                    {
                        manager->mouseButtonEvent(window, mouseButton_t::right, buttonState_t::down);
                    }
                    break;
                }

                case WM_RBUTTONUP:
                {
                    window->mouseButton[(unsigned int)mouseButton_t::right] = buttonState_t::up;

                    if (manager->mouseButtonEvent != nullptr)
                    {
                        manager->mouseButtonEvent(window, mouseButton_t::right, buttonState_t::up);
                    }
                    break;
                }

                case WM_MBUTTONDOWN:
                {
                    window->mouseButton[(unsigned int)mouseButton_t::middle] = buttonState_t::down;

                    if (manager->mouseButtonEvent != nullptr)
                    {
                        manager->mouseButtonEvent(window, mouseButton_t::middle, buttonState_t::down);
                    }
                    break;
                }

                case WM_MBUTTONUP:
                {
                    window->mouseButton[(unsigned int)mouseButton_t::middle] = buttonState_t::up;

                    if (manager->mouseButtonEvent != nullptr)
                    {
                        manager->mouseButtonEvent(window, mouseButton_t::middle, buttonState_t::up);
                    }
                    break;
                }

                case WM_MOUSEWHEEL:
                {
                    int delta = GET_WHEEL_DELTA_WPARAM(wordParam);
                    if (delta > 0)
                    {
                        //if was previously negative, revert to zero
                        if (window->accumWheelDelta < 0)
                        {
                            window->accumWheelDelta = 0;
                        }

                        else
                        {
                            window->accumWheelDelta += delta;
                        }

                        if (window->accumWheelDelta >= WHEEL_DELTA)
                        {
                            if (manager->mouseWheelEvent != nullptr)
                            {
                                manager->mouseWheelEvent(window, mouseScroll_t::up);
                            }
                            //reset accum
                            window->accumWheelDelta = 0;
                        }
                    }

                    else
                    {
                        //if was previously positive, revert to zero
                        if (window->accumWheelDelta > 0)
                        {
                            window->accumWheelDelta = 0;
                        }

                        else
                        {
                            window->accumWheelDelta += delta;
                        }

                        //if the delta is equal to or greater than delta
                        if (window->accumWheelDelta <= -WHEEL_DELTA)
                        {
                            if (manager->mouseWheelEvent != nullptr)
                            {
                                manager->mouseWheelEvent(window, mouseScroll_t::down);
                            }
                            //reset accum
                            window->accumWheelDelta = 0;
                        }
                    }
                    break;
                }

                case WM_SETFOCUS:
                {
                    window->inFocus = true;
                    if (manager->focusEvent != nullptr)
                    {
                        manager->focusEvent(window, true);
                    }
                    break;
                }

                case WM_KILLFOCUS:
                {
                    window->inFocus = false;
                    if (manager->focusEvent != nullptr)
                    {
                        manager->focusEvent(window, false);
                    }
                    break;
                }

                case WM_DROPFILES:
                {
                    //get the number of files that were dropped
                    unsigned int numfilesDropped = DragQueryFile((HDROP)wordParam, 0xFFFFFFFF, nullptr, 0);
                    std::vector<std::string> files;
                    
                    //for each file dropped store the path
                    for (size_t fileIter = 0; fileIter < numfilesDropped; fileIter++)
                    {
                        char file[255] = {0};
                        unsigned int stringSize = DragQueryFile((HDROP)wordParam, (UINT)fileIter, nullptr, 0); //get the size of the string
                        DragQueryFile((HDROP)wordParam, (UINT)fileIter, file, stringSize + 1);  //get the string itself
                        files.emplace_back(file);
                    }
                    POINT mousePoint;
                    vec2_t<int> mousePosition;
                    if (DragQueryPoint((HDROP)wordParam, &mousePoint)) //get the mouse position where the file was dropped
                    {
                        mousePosition = vec2_t<int>(mousePoint.x, mousePoint.y);
                    }

                    //release the memory
                    DragFinish((HDROP)wordParam);

                    if (manager->fileDropEvent != nullptr)
                    {
                        manager->fileDropEvent(window, std::move(files), mousePosition);
                    }
                    break;
                }

                default:
                {
                    return DefWindowProc(windowHandle, winMessage, wordParam, longParam);
                }
            }

            return 0;
        }

        //user data should be a pointer to a window manager
        static BOOL CALLBACK MonitorEnumProcedure(HMONITOR monitorHandle, HDC monitorDeviceContextHandle, LPRECT monitorSize, LPARAM userData)
        {
            windowManager* manager = (windowManager*)userData;
            MONITORINFOEX info = {};
            info.cbSize = sizeof(info);
            GetMonitorInfo(monitorHandle, &info);
            
            monitor_t* monitor = manager->GetMonitorByHandle(info.szDevice);
            monitor->monitorHandle = monitorHandle;
            monitor->extents = vec4_t<int>(monitorSize->left, monitorSize->top, monitorSize->right, monitorSize->bottom);
            monitor->resolution.width = abs(monitor->extents.right - monitor->extents.left);
            monitor->resolution.height = abs(monitor->extents.bottom - monitor->extents.top);
            return true;
        }

        //get the window that is associated with this Win32 window handle
        tWindow* GetWindowByHandle(HWND windowHandle)
        {
            for (auto & windowIndex : windowList)
            {
                if (windowIndex->windowHandle == windowHandle)
                {
                    return windowIndex.get();
                }
            }
            return nullptr;
        }

        monitor_t* GetMonitorByHandle(std::string const &displayName)
        {
            for (auto & iter : monitorList)
            {
                if (displayName.compare(iter->displayName) == 0)
                {
                    return iter;
                }
            }
            return nullptr;
        }

        //initialize the given window using Win32
        void Windows_InitializeWindow(tWindow* window,
            UINT style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW,
            int clearScreenExtra = 0, int windowExtra = 0,
            HINSTANCE winInstance = GetModuleHandle(nullptr),
            HICON icon = LoadIcon(nullptr, IDI_APPLICATION),
            HCURSOR cursor = LoadCursor(nullptr, IDC_ARROW),
            HBRUSH brush = (HBRUSH)GetStockObject(WHITE_BRUSH))
        {
            window->instanceHandle = winInstance;
            window->windowClass.style = style;
            window->windowClass.lpfnWndProc = windowManager::WindowProcedure;
            window->windowClass.cbClsExtra = clearScreenExtra;
            window->windowClass.cbWndExtra = windowExtra;
            window->windowClass.hInstance = window->instanceHandle;
            window->windowClass.hIcon = icon;
            window->windowClass.hCursor = cursor;
            window->windowClass.hbrBackground = brush;
            window->windowClass.lpszMenuName = window->settings.name;
            window->windowClass.lpszClassName = window->settings.name;
            RegisterClass(&window->windowClass);

            window->windowHandle =
                CreateWindow(window->settings.name, window->settings.name, WS_OVERLAPPEDWINDOW, 0,
                0, window->settings.resolution.width,
                window->settings.resolution.height,
                nullptr, nullptr, nullptr, nullptr);

            SetWindowLongPtr(window->windowHandle, GWLP_USERDATA, (LONG_PTR)this);

            //if TW_USE_VULKAN is defined then stop TinyWindow from creating an OpenGL context since it will conflict with a vulkan context
#if !defined(TW_USE_VULKAN)
            InitializeGL(window);
#endif
            ShowWindow(window->windowHandle, 1);
            UpdateWindow(window->windowHandle);

            CheckWindowScreen(window);

            //get screen by window Handle

            window->SetStyle(style_t::normal);

            DragAcceptFiles(window->windowHandle, true);
        }

        std::error_code Windows_CreateDummyWindow()
        {
            dummyWindowInstance = GetModuleHandle(nullptr);
            WNDCLASS dummyClass;
            dummyClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
            dummyClass.lpfnWndProc = windowManager::WindowProcedure;
            dummyClass.cbClsExtra = 0;
            dummyClass.cbWndExtra = 0;
            dummyClass.hInstance = dummyWindowInstance;
            dummyClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
            dummyClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            dummyClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
            dummyClass.lpszMenuName = "dummy";
            dummyClass.lpszClassName = "dummy";
            RegisterClass(&dummyClass);

            dummyWindowHandle = CreateWindow("dummy", "dummy", WS_OVERLAPPEDWINDOW,
                0, 0, 1, 1, nullptr, nullptr, nullptr, nullptr);
            if (dummyWindowHandle == nullptr)
            {
                return error_t::invalidDummyWindow; 
            }

            ShowWindow(dummyWindowHandle, SW_HIDE);

            return error_t::success;
        }

        //initialize the pixel format for the selected window
        void InitializePixelFormat(tWindow* window)
        {
            unsigned int count = WGL_NUMBER_PIXEL_FORMATS_ARB;
            int format = 0;
            int attribs[] =
            {
                WGL_SUPPORT_OPENGL_ARB, 1,
                WGL_DRAW_TO_WINDOW_ARB, 1,
                WGL_DOUBLE_BUFFER_ARB, 1,
                WGL_RED_BITS_ARB, window->settings.colorBits,
                WGL_GREEN_BITS_ARB, window->settings.colorBits,
                WGL_BLUE_BITS_ARB, window->settings.colorBits,
                WGL_ALPHA_BITS_ARB, window->settings.colorBits,
                WGL_DEPTH_BITS_ARB, window->settings.depthBits,
                WGL_STENCIL_BITS_ARB, window->settings.stencilBits,
                WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
                WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB
            };

            std::vector<int> attribList;
            attribList.assign(attribs, attribs + std::size(attribs));


            if (wglChoosePixelFormatARB != nullptr)
            {
                if(window->settings.enableSRGB)
                {
                    attribList.push_back(WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB);
                }

                attribList.push_back(0); //needs a 0 to notify as the end of the list.
                wglChoosePixelFormatARB(window->deviceContextHandle,
                    &attribList[0], nullptr, 1, &format, &count);
                SetPixelFormat(window->deviceContextHandle, format,
                    &window->pixelFormatDescriptor);
            }

            else if (wglChoosePixelFormatEXT != nullptr)
            {
                if (window->settings.enableSRGB)
                {
                    attribList.push_back(WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT);
                }

                attribList.push_back(0);
                wglChoosePixelFormatEXT(window->deviceContextHandle, 
                    &attribList[0], nullptr, 1, &format, &count);
                SetPixelFormat(window->deviceContextHandle, format,
                    &window->pixelFormatDescriptor);
            }

            else
            {
                PIXELFORMATDESCRIPTOR pfd = {};
                formatSetting_t* desiredSetting = new formatSetting_t(window->settings.colorBits, window->settings.colorBits, window->settings.colorBits, window->settings.colorBits, window->settings.depthBits, window->settings.stencilBits);
                unsigned int bestPFDHandle = GetLegacyPFD(desiredSetting, window->deviceContextHandle)->handle;
                if (!DescribePixelFormat(window->deviceContextHandle, bestPFDHandle, sizeof(pfd), &pfd))
                {
                    return;
                }
                SetPixelFormat(window->deviceContextHandle, bestPFDHandle, &pfd);
            }
        }

        formatSetting_t* GetLegacyPFD(formatSetting_t* desiredSetting, HDC deviceContextHandle)
        {
            //use the old PFD system on the window if none of the extensions will load  
            int nativeCount = 0;
            int numCompatible = 0;
            //pass nullptr to get the total number of PFDs that are available
            nativeCount = DescribePixelFormat(deviceContextHandle, 1, sizeof(PIXELFORMATDESCRIPTOR), nullptr);

            for (int nativeIter = 0; nativeIter < nativeCount; nativeIter++)
            {
                const int num = nativeIter + 1;
                PIXELFORMATDESCRIPTOR pfd;
                if (!DescribePixelFormat(deviceContextHandle, num, sizeof(PIXELFORMATDESCRIPTOR), &pfd))
                {
                    continue;
                }

                //skip if the PFD does not have PFD_DRAW_TO_WINDOW and PFD_SUPPORT_OPENGL 
                if (!(pfd.dwFlags & PFD_DRAW_TO_WINDOW) || !(pfd.dwFlags & PFD_SUPPORT_OPENGL))
                {
                    continue;
                }

                //skip if the PFD does not have PFD_GENERIC_ACCELERATION and PFD_GENERIC FORMAT
                if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED) &&
                    (pfd.dwFlags & PFD_GENERIC_FORMAT))
                {
                    continue;
                }

                //if the pixel type is not RGBA
                if (pfd.iPixelType != PFD_TYPE_RGBA)
                {
                    continue;
                }

                formatSetting_t* setting = new formatSetting_t(pfd.cRedBits, pfd.cGreenBits, pfd.cBlueBits, pfd.cAlphaBits,
                    pfd.cDepthBits, pfd.cStencilBits,
                    pfd.cAccumRedBits, pfd.cAccumGreenBits, pfd.cAccumBlueBits, pfd.cAccumAlphaBits,
                    pfd.cAuxBuffers, (pfd.dwFlags & PFD_STEREO) ? true : false, (pfd.dwFlags & PFD_DOUBLEBUFFER) ? true : false);
                setting->handle = num;

                formatList.push_back(std::move(setting));
                numCompatible++;
            }

            if (numCompatible == 0)
            {
                //need to add an error message pipeline to this.
                //or a list of messages with a function to get last error
                //your driver has no compatible PFDs for OpenGL. at all. the fuck?
                return nullptr;
            }

            //the best PFD would probably be the most basic by far
            formatSetting_t defaultSetting = formatSetting_t();
            defaultSetting.redBits = 8;
            defaultSetting.greenBits = 8;
            defaultSetting.blueBits = 8;
            defaultSetting.alphaBits = 8;
            defaultSetting.depthBits = 24;
            defaultSetting.stencilBits = 8;
            defaultSetting.doubleBuffer = true;

            //if the best format hasn't already been found then find them manually
            formatSetting_t* bestFormat = GetClosestFormat(desiredSetting);
            if (!bestFormat)
            {
                return nullptr;
            }
            return bestFormat;
        }

        formatSetting_t* GetClosestFormat(const formatSetting_t* desiredFormat)
        {
            //go through all the compatible format settings 
            unsigned int absent, lowestAbsent = UINT_MAX;
            unsigned int colorDiff, lowestColorDiff = UINT_MAX;
            unsigned int extraDiff, lowestExtraDiff = UINT_MAX;
            formatSetting_t* currentFormat;
            formatSetting_t* closestFormat = nullptr;

            for (auto formatIter : formatList)
            {
                currentFormat = formatIter;
                
                //must have the same stereoscopic setting
                if (desiredFormat->stereo && !currentFormat->stereo)
                {
                    continue;
                }

                //must have the same double buffer setting
                if (desiredFormat->doubleBuffer != currentFormat->doubleBuffer)
                {
                    continue;
                }

                //get the missing buffers
                {
                    absent = 0;

                    //if the current format doesn't have any alpha bits and the desired has over 0
                    if (desiredFormat->alphaBits && !currentFormat->alphaBits)
                    {
                        absent++;
                    }
                    //if the current format doesn't have any depth bits and the desired has over 0
                    if (desiredFormat->depthBits && !currentFormat->depthBits)
                    {
                        absent++;
                    }
                    //if the current format doesn't have any stencil bits and the desired has over 0
                    if (desiredFormat->stencilBits && !currentFormat->stencilBits)
                    {
                        absent++;
                    }
                    //if the desired has aux buffers and the desired has more aux buffers than the current
                    if (desiredFormat->auxBuffers && currentFormat->auxBuffers < desiredFormat->auxBuffers)
                    {
                        //add up the missing buffers as the difference in buffers between desired and current in aux buffer count
                        absent += desiredFormat->auxBuffers - currentFormat->auxBuffers;
                    }

                    //for modern framebuffers.if the desired needs samples and the current has not samples
                    if (desiredFormat->numSamples > 0 && !currentFormat->numSamples)
                    {
                        absent++;
                    }
                }

                //gather the value differences in color channels
                {
                    colorDiff = 0;

                    if (desiredFormat->redBits != -1)
                    {
                        colorDiff += (unsigned int)pow((desiredFormat->redBits - currentFormat->redBits), 2);
                    }

                    if (desiredFormat->greenBits != -1)
                    {
                        colorDiff += (unsigned int)pow((desiredFormat->greenBits - currentFormat->greenBits), 2);
                    }

                    if (desiredFormat->blueBits != -1)
                    {
                        colorDiff += (unsigned int)pow((desiredFormat->blueBits - currentFormat->blueBits), 2);
                    }
                }

                //calculates the difference in values for extras 
                {
                    extraDiff = 0;

                    if (desiredFormat->alphaBits != -1)
                    {
                        extraDiff += (unsigned int)pow((desiredFormat->alphaBits - currentFormat->alphaBits), 2);
                    }

                    if (desiredFormat->depthBits != -1)
                    {
                        extraDiff += (unsigned int)pow((desiredFormat->depthBits - currentFormat->depthBits), 2);
                    }

                    if (desiredFormat->stencilBits != -1)
                    {
                        extraDiff += (unsigned int)pow((desiredFormat->stencilBits - currentFormat->stencilBits), 2);
                    }

                    if (desiredFormat->accumRedBits != -1)
                    {
                        extraDiff += (unsigned int)pow((desiredFormat->accumRedBits - currentFormat->accumRedBits), 2);
                    }

                    if (desiredFormat->accumGreenBits != -1)
                    {
                        extraDiff += (unsigned int)pow((desiredFormat->accumGreenBits - currentFormat->accumGreenBits), 2);
                    }

                    if (desiredFormat->accumBlueBits != -1)
                    {
                        extraDiff += (unsigned int)pow((desiredFormat->accumBlueBits - currentFormat->accumBlueBits), 2);
                    }

                    if (desiredFormat->numSamples != -1)
                    {
                        extraDiff += (unsigned int)pow((desiredFormat->numSamples - currentFormat->numSamples), 2);
                    }

                    if (desiredFormat->alphaBits != -1)
                    {
                        extraDiff += (unsigned int)pow((desiredFormat->alphaBits - currentFormat->alphaBits), 2);
                    }

                    if (desiredFormat->pixelRGB && !currentFormat->pixelRGB)
                    {
                        extraDiff++;
                    }
                }

                //determine if the current one is better than the best one so far
                if (absent < lowestAbsent)
                {
                    closestFormat = currentFormat;
                }

                else if (absent == lowestAbsent)
                {
                    if ((colorDiff < lowestColorDiff) ||
                        (colorDiff == lowestColorDiff && extraDiff < lowestExtraDiff))
                    {
                        closestFormat = currentFormat;
                    }
                }

                if (currentFormat == closestFormat)
                {
                    lowestAbsent = absent;
                    lowestColorDiff = colorDiff;
                    lowestExtraDiff = extraDiff;
                }
            }
            return closestFormat;
        }
    
        void Windows_Shutown()
        {

        }

        std::error_code Windows_CreateDummyContext()
        {
            Windows_CreateDummyWindow();
            dummyDeviceContextHandle = GetDC(dummyWindowHandle);
            PIXELFORMATDESCRIPTOR pfd = {};
            formatSetting_t* desiredSetting = new formatSetting_t();
            unsigned int bestPFDHandle = GetLegacyPFD(desiredSetting, dummyDeviceContextHandle)->handle;

            if (!DescribePixelFormat(dummyDeviceContextHandle, bestPFDHandle, sizeof(PIXELFORMATDESCRIPTOR), &pfd))
            {
                return error_t::invalidDummyPixelFormat;
            }

            if (!SetPixelFormat(dummyDeviceContextHandle, bestPFDHandle, &pfd))
            {
                return error_t::invalidDummyPixelFormat;
            }

            dummyGLContextHandle = wglCreateContext(dummyDeviceContextHandle);
            if (!dummyGLContextHandle)
            {
                return error_t::dummyCreationFailed;
            }

            if (!wglMakeCurrent(dummyDeviceContextHandle, dummyGLContextHandle))
            {
                return error_t::dummyCannotMakeCurrent;
            }
            return error_t::success;
        }

        static int RetrieveDataFromWin32Pointer(LPARAM longParam, unsigned int depth)
        {
            return (longParam >> depth) & ((1L << sizeof(longParam)) - 1);
        }

        static WPARAM DetermineLeftOrRight(WPARAM key, LPARAM longParam)
        {
            std::bitset<32> bits(longParam);
            WPARAM newKey = key;
            //extract data at the 16th bit point to retrieve the scancode
            UINT scancode = RetrieveDataFromWin32Pointer(longParam, 16);
            //extract at the 24th bit point to determine if it is an extended key
            int extended = bits.test(24) != 0;

            switch (key) {
            case VK_SHIFT:
                newKey = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
                break;
            case VK_CONTROL:
                newKey = extended ? VK_RCONTROL : VK_LCONTROL;
                break;
            case VK_MENU:
                newKey = extended ? VK_RMENU : VK_LMENU;
                break;
            default:
                // if it cannot determine left from right then just return the original key
                newKey = key;
                break;
            }

            return newKey;
        }

        static unsigned int Windows_TranslateKey(WPARAM wordParam)
        {
            switch (wordParam)
            {
                case VK_ESCAPE:
                {
                    return escape;
                }

                case VK_SPACE:
                {
                    return spacebar;
                }
                
                case VK_F1:
                {
                    return F1;
                }

                case VK_F2:
                {
                    return F2;
                }

                case VK_F3:
                {
                    return F3;
                }

                case VK_F4:
                {
                    return F4;
                }

                case VK_F5:
                {
                    return F5;
                }

                case VK_F6:
                {
                    return F6;
                }

                case VK_F7:
                {
                    return F7;
                }

                case VK_F8:
                {
                    return F8;
                }

                case VK_F9:
                {
                    return F9;
                }

                case VK_F10:
                {
                    return F10;
                }

                case VK_F11:
                {
                    return F11;
                }

                case VK_F12:
                {
                    return F12;
                }

                case VK_BACK:
                {
                    return backspace;
                }

                case VK_TAB:
                {
                    return tab;
                }

                case VK_CAPITAL:
                {
                    return capsLock;
                }

                case VK_RETURN:
                {
                    return enter;
                }

                case VK_PRINT:
                {
                    return printScreen;
                }

                case VK_SCROLL:
                {
                    return scrollLock;
                }

                case VK_PAUSE:
                {
                    return pause;
                }

                case VK_INSERT:
                {
                    return insert;
                }

                case VK_HOME:
                {
                    return home;
                }

                case VK_DELETE:
                {
                    return del;
                }

                case VK_END:
                {
                    return end;
                }

                case VK_PRIOR:
                {
                    return pageUp;
                }

                case VK_NEXT:
                {
                    return pageDown;
                }

                case VK_DOWN:
                {
                    return arrowDown;
                }

                case VK_UP:
                {
                    return arrowUp;
                }

                case VK_LEFT:
                {
                    return arrowLeft;
                }

                case VK_RIGHT:
                {
                    return arrowRight;
                }

                case VK_DIVIDE:
                {
                    return keypadDivide;
                }

                case VK_MULTIPLY:
                {
                    return keypadMultiply;
                }

                case VK_SUBTRACT:
                {
                    return keypadDivide;
                }

                case VK_ADD:
                {
                    return keypadAdd;
                }

                case VK_DECIMAL:
                {
                    return keypadPeriod;
                }

                case VK_NUMPAD0:
                {
                    return keypad0;
                }

                case VK_NUMPAD1:
                {
                    return keypad1;
                }

                case VK_NUMPAD2:
                {
                    return keypad2;
                }

                case VK_NUMPAD3:
                {
                    return keypad3;
                }

                case VK_NUMPAD4:
                {
                    return keypad4;
                }

                case VK_NUMPAD5:
                {
                    return keypad5;
                }

                case VK_NUMPAD6:
                {
                    return keypad6;
                }

                case VK_NUMPAD7:
                {
                    return keypad7;
                }

                case VK_NUMPAD8:
                {
                    return keypad8;
                }

                case VK_NUMPAD9:
                {
                    return keypad9;
                }

                case VK_LWIN:
                {
                    return leftWindow;
                }

                case VK_RWIN:
                {
                    return rightWindow;
                }

                default:
                {
                    return 0;
                }
            }
        }

        static void Windows_SetWindowIcon(tWindow* window, const char* icon, unsigned int width, unsigned int height)
        {
            SendMessage(window->windowHandle, (UINT)WM_SETICON, ICON_BIG, 
                (LPARAM)LoadImage(window->instanceHandle, icon, IMAGE_ICON, (int)width, (int)height, LR_LOADFROMFILE));
        }

        void Windows_GetScreenInfo()
        {
            //get display device info
            DISPLAY_DEVICE graphicsDevice;
            graphicsDevice.cb = sizeof(DISPLAY_DEVICE);
            graphicsDevice.StateFlags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
            DWORD deviceNum = 0; 
            DWORD monitorNum = 0;
            while (EnumDisplayDevices(nullptr, deviceNum, &graphicsDevice, EDD_GET_DEVICE_INTERFACE_NAME))
            {
                //get monitor infor for the current display device
                DISPLAY_DEVICE monitorDevice = { 0 };
                monitorDevice.cb = sizeof(DISPLAY_DEVICE);
                monitorDevice.StateFlags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
                monitor_t* monitor = nullptr;
                
                //if it has children add them to the list, else, ignore them since those are only POTENTIAL monitors/devices
                while (EnumDisplayDevices(graphicsDevice.DeviceName, monitorNum, &monitorDevice, EDD_GET_DEVICE_INTERFACE_NAME))
                {
                    monitor = new monitor_t(graphicsDevice.DeviceName, graphicsDevice.DeviceString, monitorDevice.DeviceString, (graphicsDevice.StateFlags | DISPLAY_DEVICE_PRIMARY_DEVICE) ? true : false);
                    //get current display mode
                    DEVMODE devmode;
                    //get all display modes
                    unsigned int modeIndex = UINT_MAX;
                    while (EnumDisplaySettings(graphicsDevice.DeviceName, modeIndex, &devmode))
                    {
                        //get the current settings of the display
                        if (modeIndex == ENUM_CURRENT_SETTINGS)
                        {
                            monitor->currentSetting = new monitorSetting_t(vec2_t<unsigned int>(devmode.dmPelsWidth, devmode.dmPelsHeight), devmode.dmBitsPerPel, devmode.dmDisplayFrequency);
                            monitor->currentSetting->displayFlags = devmode.dmDisplayFlags;
                            monitor->currentSetting->fixedOutput = devmode.dmDisplayFixedOutput;
                        }
                        //get the settings that are stored in the registry
                        else
                        {
                            monitorSetting_t* newSetting = new monitorSetting_t(vec2_t<unsigned int>(devmode.dmPelsWidth, devmode.dmPelsHeight), devmode.dmBitsPerPel, devmode.dmDisplayFrequency);
                            newSetting->displayFlags = devmode.dmDisplayFlags;
                            newSetting->fixedOutput = devmode.dmDisplayFixedOutput;
                            monitor->settings.insert(monitor->settings.begin(), std::move(newSetting));
                        }
                        modeIndex++;                        
                    }
                    monitorList.push_back(std::move(monitor));
                    monitorNum++;
                    monitorDevice.StateFlags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
                }
                deviceNum++;
                monitorNum = 0;
            }
            //this is just to grab the monitor extents
            EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProcedure, (LPARAM)this);
        }

        bool Windows_ExtensionSupported(const char* extensionName)
        {
            const char* wglExtensions;

            if (wglGetExtensionsStringARB != nullptr)
            {
                wglExtensions = wglGetExtensionsStringARB(dummyDeviceContextHandle);
                if (wglExtensions != nullptr)
                {
                    if (std::strstr(wglExtensions, extensionName) != nullptr)
                    {
                        return true;
                    }
                }
            }

            if (wglGetExtensionsStringEXT != nullptr)
            {
                wglExtensions = wglGetExtensionsStringEXT();
                if (wglExtensions != nullptr)
                {
                    if (std::strstr(wglExtensions, extensionName) != nullptr)
                    {
                        return true;
                    }
                }

            }
            return false;
        }

        void Windows_ResetMonitors()
        {
            for (auto iter : monitorList)
            {
                ChangeDisplaySettingsEx(iter->displayName.c_str(), nullptr, nullptr, CDS_FULLSCREEN, nullptr);
            }
        }

        std::error_code Windows_InitExtensions()
        {
            wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
            wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
            if (wglGetExtensionsStringARB == nullptr && wglGetExtensionsStringEXT == nullptr)
            {
                return error_t::noExtensions;

            }
            wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
            wglChoosePixelFormatEXT = (PFNWGLCHOOSEPIXELFORMATEXTPROC)wglGetProcAddress("wglChoosePixelFormatEXT");
            wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
            wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
            wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");

            swapControlEXT = Windows_ExtensionSupported("WGL_EXT_swap_control");
            wglFramebufferSRGBCapableARB = Windows_ExtensionSupported("WGL_ARB_framebuffer_sRGB");

            wglGetPixelFormatAttribfvARB = (PFNWGLGETPIXELFORMATATTRIBFVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribfvARB");
            wglGetPixelFormatAttribfvEXT = (PFNWGLGETPIXELFORMATATTRIBFVEXTPROC)wglGetProcAddress("wglGetPixelFormatAttribfvEXT");
            wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribivARB");
            wglGetPixelFormatAttribivEXT = (PFNWGLGETPIXELFORMATATTRIBIVEXTPROC)wglGetProcAddress("wglGetPixelFormatAttribivEXT");

            return error_t::success;
        }

        std::error_code Windows_InitGL(tWindow* window)
        {
            window->deviceContextHandle = GetDC(window->windowHandle);
            InitializePixelFormat(window);
            if (wglCreateContextAttribsARB)
            {
                int attribs[]
                {
                    WGL_CONTEXT_MAJOR_VERSION_ARB, window->settings.versionMajor,
                    WGL_CONTEXT_MINOR_VERSION_ARB, window->settings.versionMinor,
                    WGL_CONTEXT_PROFILE_MASK_ARB, window->settings.profile,
    #if defined(_DEBUG)
                    WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
    #endif
                    0
                };

                window->glRenderingContextHandle = wglCreateContextAttribsARB(window->deviceContextHandle, nullptr, attribs);

                if (window->glRenderingContextHandle == nullptr)
                {
                    switch (GetLastError())
                    {
                    case ERROR_INVALID_VERSION_ARB:
                    {
                        return TinyWindow::error_t::invalidVersion;
                    }

                    case ERROR_INVALID_PROFILE_ARB:
                    {
                        return TinyWindow::error_t::invalidProfile;
                    }
                    }
                }
            }

            else
            {
                //use the old fashion method if the extensions aren't loading
                window->glRenderingContextHandle = wglCreateContext(window->deviceContextHandle);
            }

            wglMakeCurrent(window->deviceContextHandle, window->glRenderingContextHandle);

            window->contextCreated = (window->glRenderingContextHandle != nullptr);

            if (window->contextCreated)
            {
                return TinyWindow::error_t::success;
            }

            return TinyWindow::error_t::invalidContext;
        }

        void Windows_ShareContexts(tWindow* sourceWindow, tWindow* newWindow)
        {
            wglShareLists(sourceWindow->glRenderingContextHandle, newWindow->glRenderingContextHandle);
        }

        void ShutdownDummy()
        {
            if (dummyGLContextHandle)
            {
                wglMakeCurrent(nullptr, nullptr);
                wglDeleteContext(dummyGLContextHandle);
            }

            ReleaseDC(dummyWindowHandle, dummyDeviceContextHandle);
            UnregisterClass("dummy", dummyWindowInstance);

            FreeModule(dummyWindowInstance);

            dummyDeviceContextHandle = nullptr;
            dummyWindowHandle = nullptr;
            dummyGLContextHandle = nullptr;
        }

        void Windows_InitGamepad()
        {
            DWORD result;
            for(size_t iter = 0; iter < XUSER_MAX_COUNT; iter++)
            {
                XINPUT_STATE state;
                ZeroMemory(&state, sizeof(XINPUT_STATE));
                result = XInputGetState((DWORD)iter, &state);

                XINPUT_CAPABILITIES caps;
                XInputGetCapabilities((DWORD)iter, XINPUT_FLAG_GAMEPAD, &caps);

                gamepadList[iter] = new gamepad_t();

                Windows_FillGamepad(state, iter);

                JOYCAPS joycaps;
                ZeroMemory(&joycaps, sizeof(joycaps));
                joyGetDevCaps(iter, &joycaps, sizeof(joycaps));

                if(result == ERROR_SUCCESS)
                {
                    if(caps.Flags & XINPUT_CAPS_FFB_SUPPORTED)
                    {
                        printf("has force feedback \n");
                    }

                    if(caps.Flags & XINPUT_CAPS_WIRELESS)
                    {
                        printf("is wireless \n");
                    }


                    printf("sending force feedback \n");
                    XINPUT_VIBRATION vib;
                    ZeroMemory(&vib, sizeof(XINPUT_VIBRATION));
                    vib.wLeftMotorSpeed = 65535;
                    vib.wRightMotorSpeed = 65535;               
                }

                else
                {
                    //controller not connected
                }
            }

            
        }

        void Windows_PollGamepads()
        {
            DWORD result;
            for (size_t iter = 0; iter < XUSER_MAX_COUNT; iter++)
            {
                XINPUT_STATE state;
                ZeroMemory(&state, sizeof(XINPUT_STATE));

                result = XInputGetState((DWORD)iter, &state);

                Windows_FillGamepad(state, iter);
            }
        }

        void Windows_FillGamepad(XINPUT_STATE state, size_t gamepadIter)
        {
            //ok... how do I set up a callback system with this?
            //callback per controller region?
            gamepadList[gamepadIter]->buttonStates[gamepad_t::button_t::Dpad_top] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP);
            gamepadList[gamepadIter]->buttonStates[gamepad_t::button_t::Dpad_bottom] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
            gamepadList[gamepadIter]->buttonStates[gamepad_t::button_t::Dpad_left] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
            gamepadList[gamepadIter]->buttonStates[gamepad_t::button_t::Dpad_right] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

            gamepadList[gamepadIter]->buttonStates[gamepad_t::button_t::start] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START);
            gamepadList[gamepadIter]->buttonStates[gamepad_t::button_t::select] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK);

            gamepadList[gamepadIter]->buttonStates[gamepad_t::button_t::left_stick] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
            gamepadList[gamepadIter]->buttonStates[gamepad_t::button_t::right_stick] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);

            gamepadList[gamepadIter]->buttonStates[gamepad_t::button_t::left_shoulder] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
            gamepadList[gamepadIter]->buttonStates[gamepad_t::button_t::right_shoulder] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);

            gamepadList[gamepadIter]->buttonStates[gamepad_t::button_t::face_bottom] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A);
            gamepadList[gamepadIter]->buttonStates[gamepad_t::button_t::face_right] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_B);
            gamepadList[gamepadIter]->buttonStates[gamepad_t::button_t::face_left] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X);
            gamepadList[gamepadIter]->buttonStates[gamepad_t::button_t::face_top] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y);

            gamepadList[gamepadIter]->leftTrigger = (float)state.Gamepad.bLeftTrigger / (float)std::numeric_limits<unsigned char>::max();
            gamepadList[gamepadIter]->rightTrigger = (float)state.Gamepad.bRightTrigger / (float)std::numeric_limits<unsigned char>::max();;

            //shift these values to be between 1 and -1
            gamepadList[gamepadIter]->leftStick[0] = (float)state.Gamepad.sThumbLX / (float)std::numeric_limits<short>::max();
            gamepadList[gamepadIter]->leftStick[1] = (float)state.Gamepad.sThumbLY / (float)std::numeric_limits<short>::max();

            gamepadList[gamepadIter]->rightStick[0] = (float)state.Gamepad.sThumbRX / (float)std::numeric_limits<short>::max();
            gamepadList[gamepadIter]->rightStick[1] = (float)state.Gamepad.sThumbRY / (float)std::numeric_limits<short>::max();
        }

#elif defined(TW_LINUX)

        Display*            currentDisplay;
        XEvent              currentEvent;

        tWindow* GetWindowByHandle(Window windowHandle)
        {
            for(unsigned int windowIndex = 0; windowIndex < windowList.size(); windowIndex++)
            {
                if (windowList[windowIndex]->windowHandle == windowHandle)
                {
                    return windowList[windowIndex].get();
                }
            }
            return nullptr;
        }

        tWindow* GetWindowByEvent(XEvent currentEvent)
        {
            switch(currentEvent.type)
            {
                case Expose:
                {
                    return GetWindowByHandle(currentEvent.xexpose.window);
                }   

                case DestroyNotify:
                {
                    return GetWindowByHandle(currentEvent.xdestroywindow.window);
                }

                case CreateNotify:
                {
                    return GetWindowByHandle(currentEvent.xcreatewindow.window);
                }   

                case KeyPress:
                {
                    return GetWindowByHandle(currentEvent.xkey.window);
                }

                case KeyRelease:
                {
                    return GetWindowByHandle(currentEvent.xkey.window);
                }

                case ButtonPress:
                {
                    return GetWindowByHandle(currentEvent.xbutton.window);
                }

                case ButtonRelease:
                {
                    return GetWindowByHandle(currentEvent.xbutton.window);
                }

                case MotionNotify:
                {
                    return GetWindowByHandle(currentEvent.xmotion.window);
                }   

                case FocusIn:
                {
                    return GetWindowByHandle(currentEvent.xfocus.window);
                }

                case FocusOut:
                {
                    return GetWindowByHandle(currentEvent.xfocus.window);
                }

                case ResizeRequest:
                {
                    return GetWindowByHandle(currentEvent.xresizerequest.window);
                }

                case ConfigureNotify:
                {
                    return GetWindowByHandle(currentEvent.xconfigure.window);
                }

                case PropertyNotify:
                {
                    return GetWindowByHandle(currentEvent.xproperty.window);
                }

                case GravityNotify:
                {
                    return GetWindowByHandle(currentEvent.xgravity.window);
                }

                case ClientMessage:
                {
                    return GetWindowByHandle(currentEvent.xclient.window);
                }

                case VisibilityNotify:
                {
                    return GetWindowByHandle(currentEvent.xvisibility.window);
                }   

                default:
                {
                    return nullptr;
                }
            }
        }
    
        std::error_code Linux_InitializeWindow(tWindow* window)
        {
            window->attributes = new int[ 5]{
                GLX_RGBA,
                GLX_DOUBLEBUFFER, 
                GLX_DEPTH_SIZE, 
                window->depthBits, 
                None};

            window->linuxDecorators = 1;
            window->currentStyle |= window->linuxClose | window->linuxMaximize | window->linuxMinimize | window->linuxMove;

            if (!currentDisplay)
            {
                return TinyWindow::error_t::linuxCannotConnectXServer;
            }

            //window->VisualInfo = glXGetVisualFromFBConfig(GetDisplay(), GetBestFrameBufferConfig(window)); 

            window->visualInfo = glXChooseVisual(currentDisplay, 0, window->attributes);

            if (!window->visualInfo)
            {
                return TinyWindow::error_t::linuxInvalidVisualinfo;
            }

            window->setAttributes.colormap = XCreateColormap(currentDisplay,
                DefaultRootWindow(currentDisplay),
                window->visualInfo->visual, AllocNone);

            window->setAttributes.event_mask = ExposureMask | KeyPressMask 
                | KeyReleaseMask | MotionNotify | ButtonPressMask | ButtonReleaseMask
                | FocusIn | FocusOut | Button1MotionMask | Button2MotionMask | Button3MotionMask | 
                Button4MotionMask | Button5MotionMask | PointerMotionMask | FocusChangeMask
                | VisibilityChangeMask | PropertyChangeMask | SubstructureNotifyMask;
        
            window->windowHandle = XCreateWindow(currentDisplay,
                XDefaultRootWindow(currentDisplay), 0, 0,
                window->resolution.width, window->resolution.height,
                0, window->visualInfo->depth, InputOutput,
                window->visualInfo->visual, CWColormap | CWEventMask,
                &window->setAttributes);

            if(!window->windowHandle)
            {
                return TinyWindow::error_t::linuxCannotCreateWindow;
                exit(0);
            }

            XMapWindow(currentDisplay, window->windowHandle);
            XStoreName(currentDisplay, window->windowHandle,
                window->name);

            XSetWMProtocols(currentDisplay, window->windowHandle, &window->AtomClose, true);    

            window->currentDisplay = currentDisplay;
            InitializeGL(window);
            
            return TinyWindow::error_t::success;
        }

        void Linux_ShutdownWindow(tWindow* window)
        {
            XDestroyWindow(currentDisplay, window->windowHandle);   
        }

        void Linux_Shutdown()
        {
            for(unsigned int windowIndex = 0; windowIndex < windowList.size(); windowIndex++)
            {
                Linux_ShutdownWindow(windowList[windowIndex].get());
            }

            XCloseDisplay(currentDisplay);
        }

        void Linux_ProcessEvents(XEvent currentEvent)
        {
            tWindow* window = GetWindowByEvent(currentEvent);

            switch (currentEvent.type)  
            {
                case Expose:
                {
                    break;
                }

                case DestroyNotify:
                {
                    if (destroyedEvent != nullptr)
                    {
                        destroyedEvent(window);
                    }

                    ShutdownWindow(window);
                    break;
                }

                /*case CreateNotify:
                {
                l_Window->InitializeGL();

                if(IsValid(l_Window->m_OnCreated))
                {
                l_Window->m_OnCreated();
                }

                break;
                } */

                case KeyPress:
                {
                    unsigned int functionKeysym = XkbKeycodeToKeysym(
                        currentDisplay, currentEvent.xkey.keycode, 0, currentEvent.xkey.state & ShiftMask ? 1 : 0);

                        unsigned int translatedKey = Linux_TranslateKey(functionKeysym); 
                        window->keys[ translatedKey] = keyState_t::down;
                        if (keyEvent != nullptr)
                        {
                            keyEvent(window, translatedKey, keyState_t::down);
                        }

                    break;
                }

                case KeyRelease:
                {
                    bool isRetriggered = false;
                    if (XEventsQueued(currentDisplay, QueuedAfterReading))
                    {
                        XEvent nextEvent;
                        XPeekEvent(currentDisplay, &nextEvent);

                        if (nextEvent.type == KeyPress &&
                            nextEvent.xkey.time == currentEvent.xkey.time &&
                            nextEvent.xkey.keycode == currentEvent.xkey.keycode)
                        {
                            unsigned int functionKeysym = XkbKeycodeToKeysym(
                                currentDisplay, currentEvent.xkey.keycode, 0, 
                                currentEvent.xkey.state & ShiftMask ? 1 : 0);

                            XNextEvent(currentDisplay, &currentEvent);
                            isRetriggered = true;
                            if(keyEvent != nullptr)
                            {
                                keyEvent(window, Linux_TranslateKey(functionKeysym), keyState_t::down);
                            }
                        }
                    }

                    if (!isRetriggered)
                    {
                        unsigned int functionKeysym = XkbKeycodeToKeysym(
                        currentDisplay, currentEvent.xkey.keycode, 0, currentEvent.xkey.state & ShiftMask ? 1 : 0);

                        unsigned int translatedKey = Linux_TranslateKey(functionKeysym); 
                        window->keys[ translatedKey] = keyState_t::up;
                        if (keyEvent != nullptr)
                        {
                            keyEvent(window, translatedKey, keyState_t::up);
                        }
                    }

                    break;
                }

                case ButtonPress:
                {
                    switch (currentEvent.xbutton.button)
                    {
                    case 1:
                    {
                        window->mouseButton[ (unsigned int)mouseButton_t::left] = buttonState_t::down;

                        if (mouseButtonEvent != nullptr)
                        {
                            mouseButtonEvent(window, mouseButton_t::left, buttonState_t::down);
                        }
                        break;
                    }

                    case 2:
                    {
                        window->mouseButton[ (unsigned int)mouseButton_t::middle] = buttonState_t::down;

                        if (mouseButtonEvent != nullptr)
                        {
                            mouseButtonEvent(window, mouseButton_t::middle, buttonState_t::down);
                        }
                        break;
                    }

                    case 3:
                    {
                        window->mouseButton[ (unsigned int)mouseButton_t::right] = buttonState_t::down;

                        if (mouseButtonEvent != nullptr)
                        {
                            mouseButtonEvent(window, mouseButton_t::right, buttonState_t::down);
                        }
                        break;
                    }

                    case 4:
                    {
                        window->mouseButton[ (unsigned int)mouseScroll_t::up] = buttonState_t::down;

                        if (mouseWheelEvent != nullptr)
                        {
                            mouseWheelEvent(window, mouseScroll_t::down);
                        }
                        break;
                    }

                    case 5:
                    {
                        window->mouseButton[ (unsigned int)mouseScroll_t::down] = buttonState_t::down;

                        if (mouseWheelEvent != nullptr)
                        {
                            mouseWheelEvent(window, mouseScroll_t::down);
                        }
                        break;
                    }

                    default:
                    {
                        //need to add more mouse buttons 
                        break;
                    }
                    }

                    break;
                }

                case ButtonRelease:
                {
                    switch (currentEvent.xbutton.button)
                    {
                    case 1:
                    {
                        //the left mouse button was released
                        window->mouseButton[ (unsigned int)mouseButton_t::left] = buttonState_t::up;

                        if (mouseButtonEvent != nullptr)
                        {
                            mouseButtonEvent(window, mouseButton_t::left, buttonState_t::up);
                        }
                        break;
                    }

                    case 2:
                    {
                        //the middle mouse button was released
                        window->mouseButton[ (unsigned int)mouseButton_t::middle] = buttonState_t::up;

                        if (mouseButtonEvent != nullptr)
                        {
                            mouseButtonEvent(window, mouseButton_t::middle, buttonState_t::up);
                        }
                        break;
                    }

                    case 3:
                    {
                        //the right mouse button was released
                        window->mouseButton[ (unsigned int)mouseButton_t::right] = buttonState_t::up;

                        if (mouseButtonEvent != nullptr)
                        {
                            mouseButtonEvent(window, mouseButton_t::right, buttonState_t::up);
                        }
                        break;
                    }

                    case 4:
                    {
                        //the mouse wheel was scrolled up
                        window->mouseButton[ (unsigned int)mouseScroll_t::up] = buttonState_t::down;
                        break;
                    }

                    case 5:
                    {
                        //the mouse wheel was scrolled down
                        window->mouseButton[ (unsigned int)mouseScroll_t::down] = buttonState_t::down;
                        break;
                    }

                    default:
                    {
                        //need to add more mouse buttons
                        break;
                    }
                    }
                    break;
                }

                //when the mouse/pointer device is moved
                case MotionNotify:
                {
                    //set the windows mouse position to match the event
                    window->mousePosition.x =
                        currentEvent.xmotion.x;

                    window->mousePosition.y =
                        currentEvent.xmotion.y;

                    ///set the screen mouse position to match the event
                    screenMousePosition.x = currentEvent.xmotion.x_root;
                    screenMousePosition.y = currentEvent.xmotion.y_root;

                    if (mouseMoveEvent != nullptr)
                    {
                        mouseMoveEvent(window,  vec2_t<int>(currentEvent.xmotion.x,
                            currentEvent.xmotion.y), vec2_t<int>(currentEvent.xmotion.x_root,
                            currentEvent.xmotion.y_root));
                    }
                    break;
                }

                //when the window goes out of focus
                case FocusOut:
                {
                    window->inFocus = false;
                    if (focusEvent != nullptr)
                    {
                        focusEvent(window, window->inFocus);
                    }
                    break;
                }

                //when the window is back in focus (use to call restore callback?)
                case FocusIn:
                {
                    window->inFocus = true;

                    if (focusEvent != nullptr)
                    {
                        focusEvent(window, window->inFocus);
                    }
                    break;
                }

                //when a request to resize the window is made either by 
                //dragging out the window or programmatically
                case ResizeRequest:
                {
                    window->resolution.width = currentEvent.xresizerequest.width;
                    window->resolution.height = currentEvent.xresizerequest.height;

                    glViewport(0, 0,
                        window->resolution.width,
                        window->resolution.height);

                    if (resizeEvent != nullptr)
                    {
                        resizeEvent(window, vec2_t<unsigned int>(currentEvent.xresizerequest.width,
                            currentEvent.xresizerequest.height));
                    }

                    break;
                }

                //when a request to configure the window is made
                case ConfigureNotify:
                {
                    glViewport(0, 0, currentEvent.xconfigure.width,
                        currentEvent.xconfigure.height);

                    //check if window was resized
                    if ((unsigned int)currentEvent.xconfigure.width != window->resolution.width
                        || (unsigned int)currentEvent.xconfigure.height != window->resolution.height)
                    {
                        if (resizeEvent != nullptr)
                        {
                            resizeEvent(window, vec2_t<unsigned int>(currentEvent.xconfigure.width, currentEvent.xconfigure.height));
                        }

                        window->resolution.width = currentEvent.xconfigure.width;
                        window->resolution.height = currentEvent.xconfigure.height;
                    }

                    //check if window was moved
                    if (currentEvent.xconfigure.x != window->position.x
                        || currentEvent.xconfigure.y != window->position.y)
                    {
                        if (movedEvent != nullptr)
                        {
                            movedEvent(window, vec2_t<int>(currentEvent.xconfigure.x, currentEvent.xconfigure.y));
                        }

                        window->position.x = currentEvent.xconfigure.x;
                        window->position.y = currentEvent.xconfigure.y;
                    }
                    break;
                }

                case PropertyNotify:
                {
                    //this is needed in order to read from the windows WM_STATE Atomic
                    //to determine if the property notify event was caused by a client
                    //iconify Event(window, minimizing the window), a maximise event, a focus 
                    //event and an attention demand event. NOTE these should only be 
                    //for eventts that are not triggered programatically 

                    Atom type;
                    int format;
                    ulong numItems, bytesAfter;
                    unsigned char* properties = nullptr;

                    XGetWindowProperty(currentDisplay, currentEvent.xproperty.window,
                        window->AtomState,
                        0, LONG_MAX, false, AnyPropertyType,
                        &type, &format, &numItems, &bytesAfter,
                        & properties);

                    if (properties && (format == 32))
                    {
                        //go through each property and match it to an existing Atomic state
                        for (unsigned int itemIndex = 0; itemIndex < numItems; itemIndex++)
                        {
                            Atom currentProperty = ((long*)(properties))[ itemIndex];

                            if (currentProperty == window->AtomHidden)
                            {
                                //window was minimized
                                if (minimizedEvent != nullptr)
                                {
                                    //if the minimized callback for the window was set                          
                                    minimizedEvent(window);
                                }
                            }

                            if (currentProperty == window->AtomMaxVert ||
                                currentProperty == window->AtomMaxVert)
                            {
                                //window was maximized
                                if (maximizedEvent != nullptr)
                                {
                                    //if the maximized callback for the window was set
                                    maximizedEvent(window);
                                }
                            }

                            if (currentProperty == window->AtomFocused)
                            {
                                //window is now in focus. we can ignore this is as FocusIn/FocusOut does this anyway
                            }

                            if (currentProperty == window->AtomDemandsAttention)
                            {
                                //the window demands user attention
                            }
                        }
                    }

                    break;
                }

                case GravityNotify:
                {
                    //this is only supposed to pop up when the parent of this window(if any) has something happen
                    //to it so that this window can react to said event as well.
                    break;
                }

                //check for events that were created by the TinyWindow manager
                case ClientMessage:
                {
                    const char* atomName = XGetAtomName(currentDisplay, currentEvent.xclient.message_type);
                    if (atomName != nullptr)
                    {
                    }

                    if ((Atom)currentEvent.xclient.data.l[0] == window->AtomClose)
                    {
                        window->shouldClose = true;
                        if(destroyedEvent != nullptr)
                        {
                            destroyedEvent(window);
                        }
                        break;  
                    }

                    //check if full screen
                    if ((Atom)currentEvent.xclient.data.l[1] == window->AtomFullScreen)
                    {
                        break;
                    }
                    break;
    
                }

                default:
                {
                    return;
                }
            }
        }

        //debugging. used to determine what type of event was generated
        static const char* Linux_GetEventType(XEvent currentEvent)
        {
            switch (currentEvent.type)
            {
            case MotionNotify:
            {
                return "Motion Notify Event\n";
            }

            case ButtonPress:
            {
                return "Button Press Event\n";
            }

            case ButtonRelease:
            {
                return "Button Release Event\n";
            }

            case ColormapNotify:
            {
                return "Color Map Notify event \n";
            }

            case EnterNotify:
            {
                return "Enter Notify Event\n";
            }

            case LeaveNotify:
            {
                return "Leave Notify Event\n";
            }

            case Expose:
            {
                return "Expose Event\n";
            }

            case GraphicsExpose:
            {
                return "Graphics expose event\n";
            }

            case NoExpose:
            {
                return "No Expose Event\n";
            }

            case FocusIn:
            {
                return "Focus In Event\n";
            }

            case FocusOut:
            {
                return "Focus Out Event\n";
            }

            case KeymapNotify:
            {
                return "Key Map Notify Event\n";
            }

            case KeyPress:
            {
                return "Key Press Event\n";
            }

            case KeyRelease:
            {
                return "Key Release Event\n";
            }

            case PropertyNotify:
            {
                return "Property Notify Event\n";
            }

            case ResizeRequest:
            {
                return "Resize Property Event\n";
            }

            case CirculateNotify:
            {
                return "Circulate Notify Event\n";
            }

            case ConfigureNotify:
            {
                return "configure Notify Event\n";
            }

            case DestroyNotify:
            {
                return "Destroy Notify Request\n";
            }

            case GravityNotify:
            {
                return "Gravity Notify Event \n";
            }

            case MapNotify:
            {
                return "Map Notify Event\n";
            }

            case ReparentNotify:
            {
                return "Reparent Notify Event\n";
            }

            case UnmapNotify:
            {
                return "Unmap notify event\n";
            }

            case MapRequest:
            {
                return "Map request event\n";
            }

            case ClientMessage:
            {
                return "Client Message Event\n";
            }

            case MappingNotify:
            {
                return "Mapping notify event\n";
            }

            case SelectionClear:
            {
                return "Selection Clear event\n";
            }

            case SelectionNotify:
            {
                return "Selection Notify Event\n";
            }

            case SelectionRequest:
            {
                return "Selection Request event\n";
            }

            case VisibilityNotify:
            {
                return "Visibility Notify Event\n";
            }

            default:
            {
                return 0;
            }
            }
        }

        //translate keys from X keys to TinyWindow Keys
        static unsigned int Linux_TranslateKey(unsigned int keySymbol)
        {
            switch (keySymbol)
            {
            case XK_Escape:
            {
                return escape;
            }

            case XK_space:
            {
                return spacebar;
            }
               
            case XK_Home:
            {
                return home;
            }

            case XK_Left:
            {
                return arrowLeft;
            }

            case XK_Right:
            {
                return arrowRight;
            }

            case XK_Up:
            {
                return arrowUp;
            }

            case XK_Down:
            {
                return arrowDown;
            }

            case XK_Page_Up:
            {
                return pageUp;
            }

            case XK_Page_Down:
            {
                return pageDown;
            }

            case XK_End:
            {
                return end;
            }

            case XK_Print:
            {
                return printScreen;
            }

            case XK_Insert:
            {
                return insert;
            }

            case XK_Num_Lock:
            {
                return numLock;
            }

            case XK_KP_Multiply:
            {
                return keypadMultiply;
            }

            case XK_KP_Add:
            {
                return keypadAdd;
            }

            case XK_KP_Subtract:
            {
                return keypadSubtract;
            }

            case XK_KP_Decimal:
            {
                return keypadPeriod;
            }

            case XK_KP_Divide:
            {
                return keypadDivide;
            }

            case XK_KP_0:
            {
                return keypad0;
            }

            case XK_KP_1:
            {
                return keypad1;
            }

            case XK_KP_2:
            {
                return keypad2;
            }

            case XK_KP_3:
            {
                return keypad3;
            }

            case XK_KP_4:
            {
                return keypad4;
            }

            case XK_KP_5:
            {
                return keypad5;
            }

            case XK_KP_6:
            {
                return keypad6;
            }

            case XK_KP_7:
            {
                return keypad7;
            }

            case XK_KP_8:
            {
                return keypad8;
            }

            case XK_KP_9:
            {
                return keypad9;
            }

            case XK_F1:
            {
                return F1;
            }

            case XK_F2:
            {
                return F2;
            }

            case XK_F3:
            {
                return F3;
            }

            case XK_F4:
            {
                return F4;
            }

            case XK_F5:
            {
                return F5;
            }

            case XK_F6:
            {
                return F6;
            }

            case XK_F7:
            {
                return F7;
            }

            case XK_F8:
            {
                return F8;
            }

            case XK_F9:
            {
                return F9;
            }

            case XK_F10:
            {
                return F10;
            }

            case XK_F11:
            {
                return F11;
            }

            case XK_F12:
            {
                return F12;
            }

            case XK_Shift_L:
            {
                return leftShift;
            }

            case XK_Shift_R:
            {
                return rightShift;
            }

            case XK_Control_R:
            {
                return rightControl;
            }

            case XK_Control_L:
            {
                return leftControl;
            }

            case XK_Caps_Lock:
            {
                return capsLock;
            }

            case XK_Alt_L:
            {
                return leftAlt;
            }

            case XK_Alt_R:
            {
                return rightAlt;
            }

            default:
            {
                return keySymbol;
            }
            }
        }

        std::error_code Linux_SetWindowIcon() /*std::unique_ptr<window_t> window, const char* icon, unsigned int width, unsigned int height */
        {
            //sorry :(
            return TinyWindow::error_t::linuxFunctionNotImplemented;
        }

        GLXFBConfig GetBestFrameBufferConfig(tWindow* window)
        {
            const int visualAttributes[ ] =
            {
                GLX_X_RENDERABLE, true,
                GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
                GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
                GLX_RED_SIZE, window->colorBits,
                GLX_GREEN_SIZE, window->colorBits,
                GLX_BLUE_SIZE, window->colorBits,
                GLX_ALPHA_SIZE, window->colorBits,
                GLX_DEPTH_SIZE, window->depthBits,
                GLX_STENCIL_SIZE, window->stencilBits,
                GLX_DOUBLEBUFFER, true,
                None
            };

            int frameBufferCount;
            unsigned int bestBufferConfig;//, bestNumSamples = 0;
            GLXFBConfig* configs = glXChooseFBConfig(currentDisplay, 0, visualAttributes, &frameBufferCount);

            for (int configIndex = 0; configIndex < frameBufferCount; configIndex++)
            {
                XVisualInfo* visualInfo = glXGetVisualFromFBConfig(currentDisplay, configs[configIndex]);

                if (visualInfo)
                {
                    int samples, sampleBuffer;
                    glXGetFBConfigAttrib(currentDisplay, configs[ configIndex], GLX_SAMPLE_BUFFERS, &sampleBuffer);
                    glXGetFBConfigAttrib(currentDisplay, configs[configIndex], GLX_SAMPLES, &samples);

                    if (sampleBuffer && samples > -1)
                    {
                        bestBufferConfig = configIndex;
                        //bestNumSamples = samples;
                    }
                }

                XFree(visualInfo);
            }

            GLXFBConfig BestConfig = configs[ bestBufferConfig];

            XFree(configs);

            return BestConfig;
        }

        std::error_code Linux_InitGL(tWindow* window)
        {
            window->context = glXCreateContext(
                currentDisplay,
                window->visualInfo,
                0,
                true);

            if (window->context)
            {
                glXMakeCurrent(currentDisplay,
                    window->windowHandle,
                    window->context);

                XWindowAttributes l_Attributes;

                XGetWindowAttributes(currentDisplay,
                    window->windowHandle, &l_Attributes);
                window->position.x = l_Attributes.x;
                window->position.y = l_Attributes.y;

                window->contextCreated = true;
                window->InitializeAtoms();
                return TinyWindow::error_t::success;
            }
            return TinyWindow::error_t::linuxCannotConnectXServer;
        }

#endif
    };
}

#endif
