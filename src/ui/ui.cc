/* ui.cc — ImGui menu bar + dialogs (Stream B.6).
 *
 * This is the ImGui boundary: ImGui's C++ API is confined here. We call ImGui
 * functions freely (calling overloaded/default-arg functions is not flagged by
 * the Orthodoxy plugin); we only avoid DECLARING forbidden constructs. Where a
 * line is still flagged, a trailing HERESY(rule) comment suppresses it.
 */
#include "minesweeper/ui.h"

#include <stdio.h>
#include <string.h>

#include "imgui.h"

void ui_actions_clear(struct UiActions *a) {
  memset(a, 0, sizeof *a);
  a->set_difficulty = -1;
  a->set_scale = 0;
}

void ui_apply_theme(void) {
  ImGui::StyleColorsLight();

  /* Capture style as a pointer (taking the address declares no reference). */
  ImGuiStyle *style = &ImGui::GetStyle();
  ImVec4 gray = ImVec4(0.75F, 0.75F, 0.75F, 1.0F);
  ImVec4 popup = ImVec4(0.75F, 0.75F, 0.75F, 1.0F);

  style->Colors[ImGuiCol_WindowBg] = gray;
  style->Colors[ImGuiCol_MenuBarBg] = gray;
  style->Colors[ImGuiCol_PopupBg] = popup;
}

float ui_menu_bar(const struct Settings *s, struct UiActions *out) {
  if (!s->menu_visible) {
    return 0.0F;
  }

  float height = 0.0F;

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Game")) {
      if (ImGui::MenuItem("New", "F2")) {
        out->new_game = true;
      }
      ImGui::Separator();

      if (ImGui::MenuItem("Beginner", NULL, s->difficulty == DIFF_BEGINNER)) {
        out->set_difficulty = DIFF_BEGINNER;
      }
      if (ImGui::MenuItem("Intermediate", NULL,
                          s->difficulty == DIFF_INTERMEDIATE)) {
        out->set_difficulty = DIFF_INTERMEDIATE;
      }
      if (ImGui::MenuItem("Expert", NULL, s->difficulty == DIFF_EXPERT)) {
        out->set_difficulty = DIFF_EXPERT;
      }
      if (ImGui::MenuItem("Custom...", NULL, s->difficulty == DIFF_CUSTOM)) {
        out->open_custom = true;
      }
      ImGui::Separator();

      if (ImGui::MenuItem("Marks (?)", NULL, s->marks)) {
        out->toggle_marks = true;
      }
      if (ImGui::MenuItem("Color", NULL, s->color)) {
        out->toggle_color = true;
      }
      if (ImGui::MenuItem("Sound", NULL, s->sound)) {
        out->toggle_sound = true;
      }

      if (ImGui::BeginMenu("Scale")) {
        for (int k = 1; k <= 4; ++k) {
          char label[8];
          snprintf(label, sizeof label, "%dx", k);
          if (ImGui::MenuItem(label, NULL, s->scale == k)) {
            out->set_scale = k;
          }
        }
        ImGui::EndMenu();
      }
      ImGui::Separator();

      if (ImGui::MenuItem("Best Times...")) {
        out->open_best = true;
      }
      ImGui::Separator();

      if (ImGui::MenuItem("Exit")) {
        out->quit = true;
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("About")) {
        out->open_about = true;
      }
      ImGui::EndMenu();
    }

    height = ImGui::GetFrameHeight();
    ImGui::EndMainMenuBar();
  }

  return height;
}

/* Clamp helper (free function, no <algorithm>). */
static int ui_clamp(int v, int lo, int hi) {
  if (v < lo) {
    return lo;
  }
  if (v > hi) {
    return hi;
  }
  return v;
}

void ui_dialogs(const struct Settings *s, struct UiActions *out,
                bool *show_custom, bool *show_best, bool *show_about,
                bool *show_name, int level_for_name) {
  (void)level_for_name;

  /* Persisted edit buffers across frames while a dialog is open. */
  static int cw = 0;
  static int ch = 0;
  static int cm = 0;
  static bool custom_seeded = false;
  static char name_buf[SCORE_NAME_MAX];
  static bool name_seeded = false;

  /* --- Custom dialog --- */
  if (*show_custom) {
    if (!custom_seeded) {
      cw = s->custom_w;
      ch = s->custom_h;
      cm = s->custom_mines;
      custom_seeded = true;
    }
    ImGui::OpenPopup("Custom Field");
  }
  if (ImGui::BeginPopupModal("Custom Field", NULL,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::InputInt("Width", &cw);
    ImGui::InputInt("Height", &ch);
    ImGui::InputInt("Mines", &cm);

    if (ImGui::Button("OK")) {
      int w = ui_clamp(cw, BOARD_MIN_W, BOARD_MAX_W);
      int h = ui_clamp(ch, BOARD_MIN_H, BOARD_MAX_H);
      int m = ui_clamp(cm, BOARD_MIN_MINES, w * h - 1);
      out->custom_applied = true;
      out->custom_w = w;
      out->custom_h = h;
      out->custom_mines = m;
      *show_custom = false;
      custom_seeded = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      *show_custom = false;
      custom_seeded = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* --- Best Times dialog --- */
  if (*show_best) {
    ImGui::OpenPopup("Best Times");
  }
  if (ImGui::BeginPopupModal("Best Times", NULL,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    static const char *level_names[LEVEL_COUNT] = {"Beginner", "Intermediate",
                                                   "Expert"};
    for (int i = 0; i < LEVEL_COUNT; ++i) {
      ImGui::Text("%-14s %3d seconds   %s", level_names[i], s->best_time[i],
                  s->best_name[i]);
    }
    ImGui::Separator();
    if (ImGui::Button("Reset")) {
      out->best_reset = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("OK")) {
      *show_best = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* --- About dialog --- */
  if (*show_about) {
    ImGui::OpenPopup("About Minesweeper");
  }
  if (ImGui::BeginPopupModal("About Minesweeper", NULL,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Minesweeper");
    ImGui::Text("Cross-platform port (Dear ImGui).");
    ImGui::Separator();
    if (ImGui::Button("OK")) {
      *show_about = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  /* --- Enter Name dialog --- */
  if (*show_name) {
    if (!name_seeded) {
      name_buf[0] = '\0';
      name_seeded = true;
    }
    ImGui::OpenPopup("Enter Name");
  }
  if (ImGui::BeginPopupModal("Enter Name", NULL,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("You have the fastest time!");
    ImGui::InputText("Name", name_buf, sizeof name_buf);
    if (ImGui::Button("OK")) {
      out->name_entered = true;
      memcpy(out->name, name_buf, sizeof out->name);
      out->name[sizeof out->name - 1] = '\0';
      *show_name = false;
      name_seeded = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}
