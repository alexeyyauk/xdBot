#include "includes.hpp"
#include "ui/record_layer.hpp"
#include "ui/game_ui.hpp"
#include "ui/clickbot_layer.hpp"
#include "ui/macro_editor.hpp"
#include "ui/render_settings_layer.hpp"
#include "hacks/layout_mode.hpp"
#include "hacks/show_trajectory.hpp"

#include <Geode/loader/GameEvent.hpp>
#include <Geode/loader/SettingV3.hpp>

static const std::vector<std::string> keybindIDs = {
    "open_menu", "toggle_recording", "toggle_playing",
    "toggle_speedhack", "toggle_frame_stepper", "step_frame",
    "toggle_render", "toggle_noclip", "show_trajectory"
};

static const char* const gameButtonIDs[6] = {
    "jump-p1", "move-left-p1", "move-right-p1",
    "jump-p2", "move-left-p2", "move-right-p2"
};

static void onKeybind(bool down, std::string_view id) {
#ifdef GEODE_IS_WINDOWS

    auto& g = Global::get();

    if (!down || (LevelEditorLayer::get() && !g.mod->getSettingValue<bool>("editor_keybinds")) || g.mod->getSettingValue<bool>("disable_keybinds"))
        return;

    if (g.state != state::recording && g.mod->getSettingValue<bool>("recording_only_keybinds"))
        return;

    if (id == "open_menu") {
        if (g.layer) {
            static_cast<RecordLayer*>(g.layer)->onClose(nullptr);
            return;
        }

        RecordLayer::openMenu();
    }

    if (id == "toggle_recording")
        Macro::toggleRecording();

    if (id == "toggle_playing")
        Macro::togglePlaying();

    if (id == "toggle_frame_stepper" && PlayLayer::get())
        Global::toggleFrameStepper();

    if (id == "step_frame")
        Global::frameStep();

    if (id == "toggle_speedhack")
        Global::toggleSpeedhack();

    if (id == "show_trajectory") {
        g.mod->setSavedValue("macro_show_trajectory", !g.mod->getSavedValue<bool>("macro_show_trajectory"));

        if (g.layer) {
            if (static_cast<RecordLayer*>(g.layer)->trajectoryToggle)
                static_cast<RecordLayer*>(g.layer)->trajectoryToggle->toggle(g.mod->getSavedValue<bool>("macro_show_trajectory"));
        }

        g.showTrajectory = g.mod->getSavedValue<bool>("macro_show_trajectory");
        if (!g.showTrajectory) ShowTrajectory::trajectoryOff();
    }

    if (id == "toggle_render" && PlayLayer::get()) {
        bool result = Renderer::toggle();

        if (result && Global::get().renderer.recording)
            Notification::create("Started Rendering", NotificationIcon::Info)->show();

        if (g.layer) {
            if (static_cast<RecordLayer*>(g.layer)->renderToggle)
                static_cast<RecordLayer*>(g.layer)->renderToggle->toggle(Global::get().renderer.recording);
        }
    }

    if (id == "toggle_noclip") {
        g.mod->setSavedValue("macro_noclip", !g.mod->getSavedValue<bool>("macro_noclip"));

        if (g.layer) {
            if (static_cast<RecordLayer*>(g.layer)->noclipToggle)
                static_cast<RecordLayer*>(g.layer)->noclipToggle->toggle(g.mod->getSavedValue<bool>("macro_noclip"));
        }
    }

#endif
}

$on_game(Loaded) {

    for (auto const& id : keybindIDs) {
        listenForKeybindSettingPresses(id, [id](Keybind const&, bool down, bool repeat, double) {
            if (repeat && id != "step_frame") return;
            onKeybind(down, id);
        });
    }

    KeybindSettingPressedEventV3().listen(
        [](std::string_view, std::string_view key, Keybind const&, bool down, bool repeat, double) {
            if (repeat) return;

            for (size_t i = 0; i < 6; i++) {
                if (key == gameButtonIDs[i])
                    Global::get().heldButtons[i] = down;
            }
        }
    ).leak();

}