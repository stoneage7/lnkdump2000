
/*****
 * From Tilemap Studio (https://github.com/Rangi42/tilemap-studio/blob/master/src/themes.h)
 * Windows-specific code was removed.
 * Brushed metal was removed due to needing a PNG.
 * Font settings were removed.
 * Licence: LGPL
 *****/

#ifndef THEMES_H
#define THEMES_H

#include <FL/Enumerations.H>

#define OS_BUTTON_UP_BOX              FL_GTK_UP_BOX
#define OS_CHECK_DOWN_BOX             FL_GTK_DOWN_BOX
#define OS_BUTTON_UP_FRAME            FL_GTK_UP_FRAME
#define OS_CHECK_DOWN_FRAME           FL_GTK_DOWN_FRAME
#define OS_PANEL_THIN_UP_BOX          FL_GTK_THIN_UP_BOX
#define OS_SPACER_THIN_DOWN_BOX       FL_GTK_THIN_DOWN_BOX
#define OS_PANEL_THIN_UP_FRAME        FL_GTK_THIN_UP_FRAME
#define OS_SPACER_THIN_DOWN_FRAME     FL_GTK_THIN_DOWN_FRAME
#define OS_RADIO_ROUND_DOWN_BOX       FL_GTK_ROUND_DOWN_BOX
#define OS_HOVERED_UP_BOX             FL_PLASTIC_UP_BOX
#define OS_DEPRESSED_DOWN_BOX         FL_PLASTIC_DOWN_BOX
#define OS_HOVERED_UP_FRAME           FL_PLASTIC_UP_FRAME
#define OS_DEPRESSED_DOWN_FRAME       FL_PLASTIC_DOWN_FRAME
#define OS_INPUT_THIN_DOWN_BOX        FL_PLASTIC_THIN_DOWN_BOX
#define OS_INPUT_THIN_DOWN_FRAME      FL_PLASTIC_ROUND_DOWN_BOX
#define OS_MINI_BUTTON_UP_BOX         FL_GLEAM_UP_BOX
#define OS_MINI_DEPRESSED_DOWN_BOX    FL_GLEAM_DOWN_BOX
#define OS_MINI_BUTTON_UP_FRAME       FL_GLEAM_UP_FRAME
#define OS_MINI_DEPRESSED_DOWN_FRAME  FL_GLEAM_DOWN_FRAME
#define OS_DEFAULT_BUTTON_UP_BOX      FL_DIAMOND_UP_BOX
#define OS_DEFAULT_HOVERED_UP_BOX     FL_PLASTIC_THIN_UP_BOX
#define OS_DEFAULT_DEPRESSED_DOWN_BOX FL_DIAMOND_DOWN_BOX
#define OS_TOOLBAR_BUTTON_HOVER_BOX   FL_GLEAM_ROUND_UP_BOX
#define OS_TABS_BOX                   FL_EMBOSSED_BOX
#define OS_SWATCH_BOX                 FL_ENGRAVED_BOX
#define OS_SWATCH_FRAME               FL_ENGRAVED_FRAME
#define OS_BG_BOX                     FL_FREE_BOXTYPE
#define OS_BG_DOWN_BOX                (Fl_Boxtype)(FL_FREE_BOXTYPE+1)
#define OS_TOOLBAR_FRAME              (Fl_Boxtype)(FL_FREE_BOXTYPE+2)

#define OS_TAB_COLOR FL_FREE_COLOR

class Themes {
public:
	enum class Theme { CLASSIC, AERO, METRO, AQUA, GREYBIRD, OCEAN, BLUE, OLIVE, ROSE_GOLD, DARK, BRUSHED_METAL, HIGH_CONTRAST };
private:
	static Theme _current_theme;
	static bool _is_consolas;
public:
	inline static Theme current_theme(void) { return _current_theme; }
	inline static constexpr bool is_dark_theme(Theme t) { return t == Theme::DARK || t == Theme::HIGH_CONTRAST; }
	static void use_native_settings(void);
	static void use_classic_theme(void);
	static void use_aero_theme(void);
	static void use_metro_theme(void);
	static void use_aqua_theme(void);
	static void use_greybird_theme(void);
	static void use_ocean_theme(void);
	static void use_blue_theme(void);
	static void use_olive_theme(void);
	static void use_rose_gold_theme(void);
	static void use_dark_theme(void);
	static void use_high_contrast_theme(void);
};

#endif
