// Author: Sean Pesce
// Original d3d9.dll wrapper base by Michael Koch

#include "stdafx.h"
#include "proxydll.h"

// Global variables
#pragma data_seg (".d3d9_shared")
SpD3D9Device*		gl_pSpD3D9Device;
SpD3D9Interface*	gl_pSpD3D9Interface;
SpD3D9SwapChain*	gl_pSpD3D9SwapChain;
DWORD				lasterr;
HINSTANCE			gl_hOriginalDll;
HINSTANCE			gl_hThisInstance;
#pragma data_seg ()

//char* const export_names[SP_D3D9_EXPORT_COUNT_] = { "D3DPERF_BeginEvent", "D3DPERF_EndEvent", "D3DPERF_GetStatus", "D3DPERF_QueryRepeatFrame", "D3DPERF_SetMarker", "D3DPERF_SetOptions", "D3DPERF_SetRegion", "DebugSetLevel", "DebugSetMute", "Direct3D9EnableMaximizedWindowedModeShim", "Direct3DCreate9", "Direct3DCreate9Ex", "Direct3DShaderValidatorCreate9", "PSGPError", "PSGPSampleTexture" };
//extern "C" UINT_PTR d3d9_original_procs[SP_D3D9_EXPORT_COUNT_] = { 0 };

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    // To avoid compiler level 4 warnings 
    LPVOID lpDummy = lpReserved;
    lpDummy = NULL;

    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            _SP_SP_D3D9_LOG_INIT_(_SP_D3D9_LOG_DEFAULT_FILE_); // Initialize log file(s)
            InitInstance(hModule);
            InitSettings();
            if (!gl_hOriginalDll)
            {
                // Get original DLL from system directory
                LoadOriginalDll();
                d3d9_dll_chain_failed = true;
            }
            // Initialize the thread for the mod:
            mod_thread = CreateThread(
                NULL,				// Default security attributes
                0,					// Use default stack size
                init_input_thread,	// Thread function name
                NULL,				// Argument to thread function
                0,					// Use default creation flags
                &mod_thread_id);	// Returns the thread identifier
            break;
        case DLL_PROCESS_DETACH:
            input_loop_enabled = false;
            if (gl_pSpD3D9Device != NULL)
            {
                delete gl_pSpD3D9Device;
                gl_pSpD3D9Device = NULL;
            }
            SpD3D9OInputHandler::delete_instance();
            _SP_D3D9_LOG_EVENT_("Exit (Detached from process)\n\n");
            ExitInstance();
            break;
        case DLL_THREAD_ATTACH:
            //_SP_D3D9_LOG_EVENT_("Thread %d start", GetCurrentThreadId());
            break;
        case DLL_THREAD_DETACH:
            //_SP_D3D9_LOG_EVENT_("Thread %d terminated", GetCurrentThreadId());
            break;
        default:
            break;
    }
    return TRUE;
}

// Exported function (faking d3d9.dll's one-and-only export)
IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion)
{
    if (!gl_hOriginalDll)
    {
        LoadOriginalDll(); // Looking for the real d3d9.dll
        d3d9_dll_chain_failed = true;
    }

    //for (int i = 0; i < SP_D3D9_EXPORT_COUNT_; i++)
    //{
    //    d3d9_original_procs[i] = (UINT_PTR)GetProcAddress(gl_hOriginalDll, (LPCSTR)export_names[i]);
    //}

    // Hooking IDirect3D Object from Original Library
    typedef IDirect3D9 *(WINAPI* D3D9_Type)(UINT SDKVersion);
    D3D9_Type D3DCreate9_func = (D3D9_Type)GetProcAddress(gl_hOriginalDll, "Direct3DCreate9");

    // Debug
    if (!D3DCreate9_func)
    {
        OutputDebugString("PROXYDLL: Pointer to original D3DCreate9 function not received ERROR ****\r\n");
        ::ExitProcess(0); // Exit the hard way
    }

    // Request pointer from Original Dll 
    IDirect3D9 *pIDirect3D9_orig = D3DCreate9_func(SDKVersion);

    // Create SpD3D9Interface object and store pointer to original object there.
    // Note: The object will delete itself once the Ref count is zero (similar to COM objects)
    gl_pSpD3D9Interface = new SpD3D9Interface(pIDirect3D9_orig);

    // Return pointer to hooking Object instead of the real one
    return (gl_pSpD3D9Interface);
}


//extern "C" __declspec(naked) void __stdcall D3DPERF_BeginEvent_jmp() { __asm {jmp d3d9_original_procs[0 * 4]} }
//extern "C" __declspec(naked) void __stdcall D3DPERF_EndEvent_jmp() { __asm {jmp d3d9_original_procs[1 * 4]} }
//extern "C" __declspec(naked) void __stdcall D3DPERF_GetStatus_jmp() { __asm {jmp d3d9_original_procs[2 * 4]} }
//extern "C" __declspec(naked) void __stdcall D3DPERF_QueryRepeatFrame_jmp() { __asm {jmp d3d9_original_procs[3 * 4]} }
//extern "C" __declspec(naked) void __stdcall D3DPERF_SetMarker_jmp() { __asm {jmp d3d9_original_procs[4 * 4]} }
//extern "C" __declspec(naked) void __stdcall D3DPERF_SetOptions_jmp() { __asm {jmp d3d9_original_procs[5 * 4]} }
//extern "C" __declspec(naked) void __stdcall D3DPERF_SetRegion_jmp() { __asm {jmp d3d9_original_procs[6 * 4]} }
//extern "C" __declspec(naked) void __stdcall DebugSetLevel_jmp() { __asm {jmp d3d9_original_procs[7 * 4]} }
//extern "C" __declspec(naked) void __stdcall DebugSetMute_jmp() { __asm {jmp d3d9_original_procs[8 * 4]} }
//extern "C" __declspec(naked) void __stdcall Direct3D9EnableMaximizedWindowedModeShim_jmp() { __asm {jmp d3d9_original_procs[9 * 4]} }
//extern "C" __declspec(naked) void __stdcall Direct3DCreate9_jmp() { __asm {jmp d3d9_original_procs[10 * 4]} }
//extern "C" __declspec(naked) void __stdcall Direct3DCreate9Ex_jmp() { __asm {jmp d3d9_original_procs[11 * 4]} }
//extern "C" __declspec(naked) void __stdcall Direct3DShaderValidatorCreate9_jmp() { __asm {jmp d3d9_original_procs[12 * 4]} }
//extern "C" __declspec(naked) void __stdcall PSGPError_jmp() { __asm {jmp d3d9_original_procs[13 * 4]} }
//extern "C" __declspec(naked) void __stdcall PSGPSampleTexture_jmp() { __asm {jmp d3d9_original_procs[14 * 4]} }


void InitInstance(HANDLE hModule)
{
    OutputDebugString("PROXYDLL: InitInstance called.\r\n");

    extern void register_default_console_commands();
    register_default_console_commands();

    // Initialization
    generic_dll_count = 0;
    gl_hOriginalDll = NULL;
    gl_hThisInstance = NULL;
    gl_pSpD3D9Interface = NULL;
    gl_pSpD3D9Device = NULL;
    gl_pSpD3D9SwapChain = NULL;

    // Get user and computer names
    char buff[MAX_PATH + 1];
    DWORD buff_size = MAX_PATH + 1;
    if (!GetComputerName(buff, &buff_size))
    {
        // Handle error
        hostname = "?Host?";
    }
    else
    {
        hostname = buff;
    }
    buff_size = MAX_PATH + 1;
    buff[0] = '\0';
    if (!GetUserName(buff, &buff_size))
    {
        // Handle error
        local_username = "?User?";
    }
    else
    {
        local_username = buff;
    }
    buff_size = MAX_PATH + 1;
    buff[0] = '\0';
    extern std::string d3d9o_dll_filename;
    if (!GetModuleFileName(NULL, buff, buff_size))
    {
        // Handle error
        game_exe_dir = "?Path?";
        d3d9o_dll_filename = "?DLL?";
    }
    else
    {
        std::string game_exe_path = buff;
        size_t file_name_start = game_exe_path.find_last_of('\\');
        game_exe_dir = game_exe_path.substr(0, file_name_start);
        d3d9o_dll_filename = game_exe_path.substr(++file_name_start);
    }

    // Storing Instance handle into global variable
    gl_hThisInstance = (HINSTANCE)hModule;
}

// Loads the original d3d9.dll from the system directory
void LoadOriginalDll(void)
{
    char buffer[MAX_PATH];

    // Getting path to system dir and to d3d9.dll
    ::GetSystemDirectory(buffer, MAX_PATH);

    // Append dll name
    strcat_s(buffer, "\\d3d9.dll");

    // Try to load the system's d3d9.dll, if pointer empty
    if (!gl_hOriginalDll) gl_hOriginalDll = ::LoadLibrary(buffer);

    // Debug
    if (!gl_hOriginalDll)
    {
        OutputDebugString("PROXYDLL: Original d3d9.dll not loaded ERROR ****\r\n");
        ::ExitProcess(0); // Exit the hard way
    }
}

// Parses settings file (.ini) for intialization settings
void InitSettings()
{
    // Get keybinds from settings file
    get_keybinds();

    // Get user preferences from settings file
    get_user_preferences();

    // Load generic DLLs
    generic_dll_count = load_generic_dlls_from_settings_file(_SP_D3D9_SETTINGS_FILE_, _SP_D3D9_SETTINGS_SECTION_ADV_SETTINGS_, _SP_D3D9_DLL_GENERIC_KEY_);

    // Chain d3d9.dll wrapper (if one is specified in the settings file)
    char d3d9_chain_buffer[128]; // Buffer to hold the filename of the d3d9.dll wrapper to load
    d3d9_chain_buffer[0] = '\0';

    // Check settings file for d3d9 DLL chain
    gl_hOriginalDll = load_dll_from_settings_file(_SP_D3D9_SETTINGS_FILE_, _SP_D3D9_SETTINGS_SECTION_ADV_SETTINGS_, _SP_D3D9_DLL_CHAIN_KEY_, d3d9_chain_buffer, 128);
    d3d9_dll_chain = std::string(d3d9_chain_buffer);
}


// Load as many generic DLLs (not wrappers) as are specified in the settings file (key numbers must be consecutive).
//	@return the number of generic DLLs that were loaded
unsigned int load_generic_dlls_from_settings_file(const char *file_name, const char *section, const char *base_key)
{
    unsigned int dll_count = 0; // Number of chained DLLs loaded
    int count_modifier = 0;
    char dll_name_buffer[128]; // Buffer to hold the filenames of the DLLs to load
    std::string load_dll_key = std::string(base_key); // Key that might hold the next d3d9.dll wrapper file name in the settings file
    HINSTANCE dll_chain_instance = NULL;

    if ((dll_chain_instance = load_dll_from_settings_file(file_name, section, load_dll_key.c_str(), dll_name_buffer, 128)) == NULL)
    {
        // First DLL entry not found
        count_modifier--;
    }

    load_dll_key = std::string(base_key).append(std::to_string(dll_count++)); // Get first numbered generic DLL settings key name

    // Load optional DLL #0
    if ((dll_chain_instance = load_dll_from_settings_file(file_name, section, load_dll_key.c_str(), dll_name_buffer, 128)) == NULL)
    {
        // Generic DLL #0 was not specified
        count_modifier--;
    }

    load_dll_key = std::string(base_key).append(std::to_string(dll_count++)); // Get next generic DLL settings key name

    if (!(dll_count + count_modifier))
    {
        return 0;
    }

    // Load all DLLs specified in the settings file
    while ((dll_chain_instance = load_dll_from_settings_file(file_name, section, load_dll_key.c_str(), dll_name_buffer, 128)) != NULL)
    {
        // Successfully loaded a DLL
        load_dll_key = std::string(base_key).append(std::to_string(dll_count++));
    }

    return dll_count + count_modifier;
}


// Loads a single DLL specified by the given settings file, section, and key parameters
HINSTANCE load_dll_from_settings_file(const char *file_name, const char *section, const char *key, char *buffer, unsigned int buffer_size)
{
    // Clear the buffer
    buffer[0] = '\0';

    // Read settings file for a DLL to load
    GetPrivateProfileString(section, key, NULL, buffer, buffer_size, file_name);

    if (buffer[0] != '\0') // Found DLL entry in settings file
    {
        HINSTANCE new_dll_instance = LoadLibrary(buffer);
        if (!new_dll_instance)
        {
            // Failed to load DLL (probably couldnt locate the file)
            return NULL;
        }
        else
        {
            // Successfully loaded the DLL

            // Register DLL plugin functions (if they exist)
            SpD3D9Overlay::load_plugin_funcs(new_dll_instance, file_name);

            return new_dll_instance;
        }
    }
    else
    {
        return NULL; // No DLL specified, or settings file was not found
    }
}


// Determines whether mod is enabled and calls the main loop for the mod
DWORD WINAPI init_input_thread(LPVOID lpParam)
{
    // @TODO: Disable mod if no mod settings are initialized?
    input_loop_enabled = true;

    extern void input_loop();	// Main loop for the mod thread
    input_loop();

    return 0;
}

// Unloads DLL when exiting
void ExitInstance()
{
    OutputDebugString("PROXYDLL: ExitInstance called.\r\n");

    // Release the system's d3d9.dll
    if (gl_hOriginalDll)
    {
        ::FreeLibrary(gl_hOriginalDll);
        gl_hOriginalDll = NULL;
    }
}

// Reads in configurable keybind values as specified in the settings file (.ini)
void get_keybinds()
{
    extern std::list<SP_KEY_FUNCTION> keybinds; // Stores all function/keybind mappings
    unsigned int key = 0;

    // Open console
    key = get_vk_hotkey(_SP_D3D9_SETTINGS_FILE_, _SP_D3D9_SETTINGS_SECTION_KEYBINDS_, _SP_D3D9_HOTKEY_OPEN_CONSOLE_KEY_);
    if (key)
    {
        extern int open_console();
        add_function_keybind(key, open_console, &keybinds);
    }
    console_key = key;
    key = 0;

    // Toggle overlay text feed
    key = get_vk_hotkey(_SP_D3D9_SETTINGS_FILE_, _SP_D3D9_SETTINGS_SECTION_KEYBINDS_, _SP_D3D9_HOTKEY_TOGGLE_OL_TXT_KEY_);
    if (key)
    {
        extern int toggle_text_feed();
        add_function_keybind(key, toggle_text_feed, &keybinds);
    }
    key = 0;

    // Toggle text feed info bar
    key = get_vk_hotkey(_SP_D3D9_SETTINGS_FILE_, _SP_D3D9_SETTINGS_SECTION_KEYBINDS_, _SP_D3D9_HOTKEY_TOGGLE_TEXT_FEED_INFO_BAR_KEY_);
    if (key)
    {
        extern int toggle_info_bar();
        add_function_keybind(key, toggle_info_bar, &keybinds);
    }
    key = 0;

    // Next overlay text feed position preset
    key = get_vk_hotkey(_SP_D3D9_SETTINGS_FILE_, _SP_D3D9_SETTINGS_SECTION_KEYBINDS_, _SP_D3D9_HOTKEY_NEXT_OL_TXT_POS_KEY_);
    if (key)
    {
        extern int next_overlay_text_position();
        add_function_keybind(key, next_overlay_text_position, &keybinds);
    }
    key = 0;

    // Next overlay text feed style (plain, shadowed, or outlined)
    key = get_vk_hotkey(_SP_D3D9_SETTINGS_FILE_, _SP_D3D9_SETTINGS_SECTION_KEYBINDS_, _SP_D3D9_HOTKEY_NEXT_OL_TXT_STYLE_KEY_);
    if (key)
    {
        extern int next_overlay_text_style();
        add_function_keybind(key, next_overlay_text_style, &keybinds);
    }
    key = 0;

    // Toggle audio feedback when mod keybinds are pressed
    key = get_vk_hotkey(_SP_D3D9_SETTINGS_FILE_, _SP_D3D9_SETTINGS_SECTION_DEV_KEYBINDS_, _SP_D3D9_HOTKEY_TOGGLE_AUDIO_FEEDBACK_KEY_);
    if (key)
    {
        extern int toggle_audio_feedback();
        add_function_keybind(key, toggle_audio_feedback, &keybinds);
    }
    key = 0;

    // Toggle verbose text feed output
    key = get_vk_hotkey(_SP_D3D9_SETTINGS_FILE_, _SP_D3D9_SETTINGS_SECTION_DEV_KEYBINDS_, _SP_D3D9_HOTKEY_TOGGLE_VERBOSE_OUTPUT_KEY_);
    if (key)
    {
        extern int toggle_verbose_output();
        add_function_keybind(key, toggle_verbose_output, &keybinds);
    }
    key = 0;

    // Print test message to overlay text feed
    key = get_vk_hotkey(_SP_D3D9_SETTINGS_FILE_, _SP_D3D9_SETTINGS_SECTION_DEV_KEYBINDS_, _SP_D3D9_HOTKEY_PRINT_OL_TXT_TEST_MSG_KEY_);
    if (key)
    {
        extern int print_overlay_test_message();
        add_function_keybind(key, print_overlay_test_message, &keybinds);
    }
    key = 0;

    // Increase text feed font size
    key = get_vk_hotkey(_SP_D3D9_SETTINGS_FILE_, _SP_D3D9_SETTINGS_SECTION_DEV_KEYBINDS_, _SP_D3D9_HOTKEY_INCREASE_TXT_SIZE_KEY_);
    if (key)
    {
        extern int increase_text_feed_font_size();
        add_function_keybind(key, increase_text_feed_font_size, &keybinds);
    }
    key = 0;

    // Decrease text feed font size
    key = get_vk_hotkey(_SP_D3D9_SETTINGS_FILE_, _SP_D3D9_SETTINGS_SECTION_DEV_KEYBINDS_, _SP_D3D9_HOTKEY_DECREASE_TXT_SIZE_KEY_);
    if (key)
    {
        extern int decrease_text_feed_font_size();
        add_function_keybind(key, decrease_text_feed_font_size, &keybinds);
    }
    key = 0;

    // Reset text feed font size
    key = get_vk_hotkey(_SP_D3D9_SETTINGS_FILE_, _SP_D3D9_SETTINGS_SECTION_DEV_KEYBINDS_, _SP_D3D9_HOTKEY_RESET_TXT_SIZE_KEY_);
    if (key)
    {
        extern int reset_text_feed_font_size();
        add_function_keybind(key, reset_text_feed_font_size, &keybinds);
    }
    key = 0;
}

// Reads in user preferences as specified in the settings file (.ini)
void get_user_preferences()
{
    // Overlay enabled/disabled
    user_pref_overlay_text_feed_enabled = ((int)GetPrivateProfileInt(_SP_D3D9_SETTINGS_SECTION_PREFS_, _SP_D3D9_OL_TXT_ENABLED_KEY_, _SP_D3D9_DEFAULT_VAL_OL_TXT_ENABLED_, _SP_D3D9_SETTINGS_FILE_) != OL_TXT_DISABLED);

    // Audio feedback enabled/disabled
    user_pref_audio_feedback_enabled = ((int)GetPrivateProfileInt(_SP_D3D9_SETTINGS_SECTION_PREFS_, _SP_D3D9_OL_TXT_AUDIO_ENABLED_KEY_, _SP_D3D9_DEFAULT_VAL_OL_TXT_AUDIO_ENABLED_, _SP_D3D9_SETTINGS_FILE_) != OL_TXT_DISABLED);

    // Verbose text output enabled/disabled
    user_pref_verbose_output_enabled = ((int)GetPrivateProfileInt(_SP_D3D9_SETTINGS_SECTION_DEV_PREFS_, _SP_D3D9_OL_TXT_VERBOSE_OUTPUT_ENABLED_KEY_, _SP_D3D9_DEFAULT_VAL_OL_TXT_VERBOSE_OUTPUT_ENABLED_, _SP_D3D9_SETTINGS_FILE_) != OL_TXT_DISABLED);

#ifdef _SP_DARK_SOULS_1_
    // PvP Watchdog overlay adjustment (Dark Souls)
    user_pref_dspw_ol_offset = ((int)GetPrivateProfileInt(_SP_D3D9_SETTINGS_SECTION_ADV_SETTINGS_, _SP_D3D9_DSPW_ADJUSTMENT_KEY_, OL_TXT_DISABLED, _SP_D3D9_SETTINGS_FILE_) != OL_TXT_DISABLED);
#endif // _SP_DARK_SOULS_1_

    // FPS counter enabled/disabled
    if ((int)GetPrivateProfileInt(_SP_D3D9_SETTINGS_SECTION_PREFS_, _SP_D3D9_OL_TXT_ENABLE_FPS_KEY_, OL_TXT_ENABLED, _SP_D3D9_SETTINGS_FILE_) != OL_TXT_DISABLED)
    {
        user_pref_show_text_feed_info_bar = SP_D3D9O_INFO_BAR_TITLE;
        user_pref_show_text_feed_info_bar += SP_D3D9O_INFO_BAR_FPS;
    }
    else
    {
        user_pref_show_text_feed_info_bar = 0;
    }

    // Gloval frame counter enabled/disabled
    if ((int)GetPrivateProfileInt(_SP_D3D9_SETTINGS_SECTION_PREFS_, _SP_D3D9_OL_TXT_ENABLE_FRAME_COUNT_KEY_, OL_TXT_DISABLED, _SP_D3D9_SETTINGS_FILE_) != OL_TXT_DISABLED)
    {
        user_pref_show_text_feed_info_bar |= SP_D3D9O_INFO_BAR_TITLE;
        user_pref_show_text_feed_info_bar += SP_D3D9O_INFO_BAR_FRAME_COUNT;
    }

    // Display date enabled/disabled
    if ((int)GetPrivateProfileInt(_SP_D3D9_SETTINGS_SECTION_PREFS_, _SP_D3D9_OL_TXT_ENABLE_DATE_KEY_, OL_TXT_ENABLED, _SP_D3D9_SETTINGS_FILE_) != OL_TXT_DISABLED)
    {
        user_pref_show_text_feed_info_bar |= SP_D3D9O_INFO_BAR_TITLE;
        user_pref_show_text_feed_info_bar += SP_D3D9O_INFO_BAR_DATE;
    }

    // Display time enabled/disabled
    if ((int)GetPrivateProfileInt(_SP_D3D9_SETTINGS_SECTION_PREFS_, _SP_D3D9_OL_TXT_ENABLE_TIME_KEY_, OL_TXT_ENABLED, _SP_D3D9_SETTINGS_FILE_) != OL_TXT_DISABLED)
    {
        user_pref_show_text_feed_info_bar |= SP_D3D9O_INFO_BAR_TITLE;
        user_pref_show_text_feed_info_bar += SP_D3D9O_INFO_BAR_TIME;
    }

    // Overlay text feed font size
    user_pref_overlay_text_size = (int)GetPrivateProfileInt(_SP_D3D9_SETTINGS_SECTION_PREFS_, _SP_D3D9_OL_TXT_SIZE_KEY_, _SP_D3D9O_TF_DEFAULT_FONT_HEIGHT_, _SP_D3D9_SETTINGS_FILE_);
    if (user_pref_overlay_text_size < 1)
    {
        // Invalid font size specified; set to default
        user_pref_overlay_text_size = _SP_D3D9O_TF_DEFAULT_FONT_HEIGHT_;
    }


    char settings_buffer[128];

    // Overlay text horizonal position
    GetPrivateProfileString(_SP_D3D9_SETTINGS_SECTION_PREFS_, _SP_D3D9_OL_TXT_HORIZONTAL_POS_KEY_, _SP_D3D9_DEFAULT_VAL_OL_TXT_HORIZONTAL_POS_, settings_buffer, 128, _SP_D3D9_SETTINGS_FILE_);
    std::string setting_value = settings_buffer;
    std::transform(setting_value.begin(), setting_value.end(), setting_value.begin(), ::toupper);
    if (strcmp(setting_value.c_str(), SP_OL_TXT_POS_VALS[OL_TXT_POS_LEFT]) == 0)
    {
        // Overlay text horizontal position will be on the left
        user_pref_overlay_text_pos = (DT_NOCLIP | DT_LEFT);
    }
    else if (strcmp(setting_value.c_str(), SP_OL_TXT_POS_VALS[OL_TXT_POS_RIGHT]) == 0)
    {
        // Overlay text horizontal position will be on the right
        user_pref_overlay_text_pos = (DT_NOCLIP | DT_RIGHT);
    }
    else
    {
        // Overlay text horizontal position will be in the center
        user_pref_overlay_text_pos = (DT_NOCLIP | DT_CENTER);
    }

    // Overlay text vertical position
    GetPrivateProfileString(_SP_D3D9_SETTINGS_SECTION_PREFS_, _SP_D3D9_OL_TXT_VERTICAL_POS_KEY_, _SP_D3D9_DEFAULT_VAL_OL_TXT_VERTICAL_POS_, settings_buffer, 128, _SP_D3D9_SETTINGS_FILE_);
    setting_value = settings_buffer;
    std::transform(setting_value.begin(), setting_value.end(), setting_value.begin(), ::toupper);
    if (strcmp(setting_value.c_str(), SP_OL_TXT_POS_VALS[OL_TXT_POS_TOP]) == 0)
    {
        // Overlay text vertical position will be on top
        user_pref_overlay_text_pos |= (DT_NOCLIP | DT_TOP);
    }
    else if (strcmp(setting_value.c_str(), SP_OL_TXT_POS_VALS[OL_TXT_POS_VCENTER]) == 0)
    {
        // Overlay text vertical position will be in the middle
        user_pref_overlay_text_pos |= (DT_NOCLIP | DT_VCENTER);
    }
    else
    {
        // Overlay text vertical position will be on the bottom
        user_pref_overlay_text_pos |= (DT_NOCLIP | DT_BOTTOM);
    }

    // Overlay text style (outlined, shadowed, or plain)
    GetPrivateProfileString(_SP_D3D9_SETTINGS_SECTION_PREFS_, _SP_D3D9_OL_TXT_STYLE_KEY_, _SP_D3D9_DEFAULT_VAL_OL_TXT_STYLE_, settings_buffer, 128, _SP_D3D9_SETTINGS_FILE_);
    setting_value = settings_buffer;
    std::transform(setting_value.begin(), setting_value.end(), setting_value.begin(), ::toupper);
    if (strcmp(setting_value.c_str(), SP_OL_TXT_STYLE_VALS[SP_D3D9O_SHADOWED_TEXT]) == 0)
    {
        // Overlay text style will be shadowed
        user_pref_overlay_text_style = SP_D3D9O_SHADOWED_TEXT;
    }
    else if (strcmp(setting_value.c_str(), SP_OL_TXT_STYLE_VALS[SP_D3D9O_PLAIN_TEXT]) == 0)
    {
        // Overlay text style will be plain
        user_pref_overlay_text_style = SP_D3D9O_PLAIN_TEXT;
    }
    else
    {
        // Overlay text style will be outlined
        user_pref_overlay_text_style = SP_D3D9O_OUTLINED_TEXT;
    }

#ifdef _SP_DARK_SOULS_1_
    // Get DSPW font size in case user wants to adjust this overlay to avoid clipping with the PvP Watchdog overlay
    dspw_pref_font_size = get_dspw_font_size();

    if (user_pref_dspw_ol_offset && dspw_pref_font_size)
    {
        // Set overlay offset to adjust for PvP Watchdog overlay
        user_pref_dspw_ol_offset = dspw_pref_font_size + 5;
    }
#endif // _SP_DARK_SOULS_1_

}

#ifdef _SP_DARK_SOULS_1_
// Reads the PvP Watchdog settings file (DSPWSteam.ini) to obtain the DSPW font size in
//  case the user wants to adjust this overlay to avoid clipping with the PvP Watchdog
//  overlay
int get_dspw_font_size()
{
    std::ifstream in("DSPWSteam.ini"); // Open DSPW settings file stream

    if (!in)
    {
        // Failed to open PvP Watchdog settings file; DSPW is probably not installed
        return 0;
    }

    // The DSPW setting we're looking for (font size)
    std::string dspw_ol_font_size_key = "FontSize ";

    std::string line;

    while (std::getline(in, line)) {

        if (line.compare(0, dspw_ol_font_size_key.length(), dspw_ol_font_size_key.c_str()) == 0)
        {
            // Found the DSPW font size key
            std::stringstream string_to_int(&line.c_str()[dspw_ol_font_size_key.length()]);
            int dspw_font_size = 0;
            if (!(string_to_int >> dspw_font_size))
            {
                // Failed to parse the DSPW font size; return default font size
                return 15;
            }
            return dspw_font_size;
        }
    }

    // Couldn't find the FontSize key in the DSPW settings file
    return 0;
}
#endif // _SP_DARK_SOULS_1_

