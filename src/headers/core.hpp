// core.hpp
#pragma once

#include <string>
#include <vector>

#include <obs.h>
#include <obs-module.h>

#include "config.hpp"

#ifndef LOG_TAG
#define LOG_TAG "[" PLUGIN_NAME "]"
#endif

#define LOGI(fmt, ...) blog(LOG_INFO,    LOG_TAG " " fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) blog(LOG_WARNING, LOG_TAG " " fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) blog(LOG_ERROR,   LOG_TAG " " fmt, ##__VA_ARGS__)

#if !defined(NDEBUG) || defined(ENABLE_LOG_DEBUG)
#define LOGD(fmt, ...) blog(LOG_INFO, LOG_TAG " [D] " fmt, ##__VA_ARGS__)
#else
#define LOGD(...) do {} while (0)
#endif

inline constexpr const char *sltBrowserSourceId   = "browser_source";
inline constexpr const char *sltBrowserSourceName = "Smart Lower Thirds";
inline constexpr int sltBrowserWidth  = 1920;
inline constexpr int sltBrowserHeight = 1080;

inline constexpr const char *sltDockId    = "smart_lt_dock";
inline constexpr const char *sltDockTitle = "Smart Lower Thirds";

namespace smart_lt {

struct lower_third_cfg {
    std::string id;

    std::string title;
    std::string subtitle;
    std::string profile_picture;

    std::string anim_in;         // animate.css class OR "custom"
    std::string anim_out;        // animate.css class OR "custom"
    std::string custom_anim_in;  // used if anim_in == "custom"
    std::string custom_anim_out; // used if anim_out == "custom"

    std::string font_family;
    std::string lt_position;     // class name: e.g. "lt-pos-bottom-left"

    std::string bg_color;
    std::string text_color;

    std::string html_template;   // inner HTML for <li id="{{ID}}">
    std::string css_template;    // should be scoped to #{{ID}} (we also do best-effort)
    std::string js_template;     // wrapped with root = document.getElementById("{{ID}}")

    std::string hotkey;
};

// -------------------------
// Output dir + startup
// -------------------------
bool has_output_dir();
std::string output_dir();
bool set_output_dir_and_load(const std::string &dir);

// Reads OBS module config: obs_module_config_path("config.json")
// Expects: {"output_dir":"C:/..."} and initializes state/visible.
// May repoint existing browser source to latest lt-*.html.
// Does NOT force rebuild on startup.
void init_from_disk();

// Save OBS module config: obs_module_config_path("config.json")
bool save_global_config();

// -------------------------
// State access
// -------------------------
std::vector<lower_third_cfg> &all();
const std::vector<lower_third_cfg> &all_const();
lower_third_cfg *get_by_id(const std::string &id);

// -------------------------
// Visible set (NO rebuild on toggle)
// -------------------------
std::vector<std::string> visible_ids();
bool is_visible(const std::string &id);
void set_visible(const std::string &id, bool visible);
void toggle_visible(const std::string &id);

// -------------------------
// Persistence
// -------------------------
bool load_state_json();
bool save_state_json();
bool load_visible_json();
bool save_visible_json();

// -------------------------
// Artifacts files
// -------------------------
bool ensure_output_artifacts_exist();
bool regenerate_merged_css_js();
std::string generate_timestamp_html();

// High-level rebuild triggers (ONLY call on: Save&Apply, add/clone/delete, output path change)
bool rebuild_and_swap();

// -------------------------
// Browser source
// -------------------------
bool ensure_browser_source_in_current_scene();
bool swap_browser_source_to_file(const std::string &absoluteHtmlPath);

// -------------------------
// Paths
// -------------------------
std::string path_state_json();     // lt-state.json
std::string path_visible_json();   // lt-visible.json
std::string path_styles_css();     // lt-styles.css
std::string path_scripts_js();     // lt-scripts.js

// -------------------------
// Utility
// -------------------------
std::string now_timestamp_string();
std::string new_id();

// -------------------------
// CRUD helpers for dock actions (rebuild triggers)
// -------------------------
std::string add_default_lower_third();                  // returns new id or ""
std::string clone_lower_third(const std::string &id);   // returns new id or ""
bool remove_lower_third(const std::string &id);         // true if removed

} // namespace smart_lt
