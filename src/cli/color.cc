#include "./color.hpp"
#include "./config.hpp"

#define CODE_TRUECOLOR_WHITE  "255;255;255"
#define CODE_TRUECOLOR_GREEN  "140;231;154"
#define CODE_TRUECOLOR_RED    "255;120;120"
#define CODE_TRUECOLOR_CYAN   "77;196;255"
#define CODE_TRUECOLOR_BLUE   "20;120;255"
#define CODE_TRUECOLOR_YELLOW "255;222;102"
#define CODE_TRUECOLOR_ORANGE "55;174;74"
#define CODE_TRUECOLOR_PURPLE "185;142;255"
#define CODE_TRUECOLOR_GREY   "165;165;165"

#define CODE_256_WHITE    "15"
#define CODE_256_GREEN    "42"
#define CODE_256_RED      "196"
#define CODE_256_BLUE     "33"
#define CODE_256_DARKBLUE "21"
#define CODE_256_YELLOW   "220"
#define CODE_256_ORANGE   "208"
#define CODE_256_PURPLE   "135"
#define CODE_256_GREY     "248"

#define FG_TRUECOLOR(val) "\033[38;2;" val "m"
#define BG_TRUECOLOR(val) "\033[48;2;" val "m"

#define FG_256COLOR(val) "\033[38;5;" val "m"
#define BG_256COLOR(val) "\033[48;5;" val "m"

#define TRUECOLOR_WHITE  FG_TRUECOLOR(CODE_TRUECOLOR_WHITE)
#define TRUECOLOR_GREEN  FG_TRUECOLOR(CODE_TRUECOLOR_GREEN)
#define TRUECOLOR_RED    FG_TRUECOLOR(CODE_TRUECOLOR_RED)
#define TRUECOLOR_CYAN   FG_TRUECOLOR(CODE_TRUECOLOR_CYAN)
#define TRUECOLOR_BLUE   FG_TRUECOLOR(CODE_TRUECOLOR_BLUE)
#define TRUECOLOR_YELLOW FG_TRUECOLOR(CODE_TRUECOLOR_YELLOW)
#define TRUECOLOR_ORANGE FG_TRUECOLOR(CODE_TRUECOLOR_ORANGE)
#define TRUECOLOR_PURPLE FG_TRUECOLOR(CODE_TRUECOLOR_PURPLE)
#define TRUECOLOR_GREY   FG_TRUECOLOR(CODE_TRUECOLOR_GREY)

#define TRUECOLOR_BG_WHITE    BG_TRUECOLOR(CODE_TRUECOLOR_WHITE)
#define TRUECOLOR_BG_GREEN    BG_TRUECOLOR(CODE_TRUECOLOR_GREEN)
#define TRUECOLOR_BG_RED      BG_TRUECOLOR(CODE_TRUECOLOR_RED)
#define TRUECOLOR_BG_BLUE     BG_TRUECOLOR(CODE_TRUECOLOR_CYAN)
#define TRUECOLOR_BG_DARKBLUE BG_TRUECOLOR(CODE_TRUECOLOR_BLUE)
#define TRUECOLOR_BG_YELLOW   BG_TRUECOLOR(CODE_TRUECOLOR_YELLOW)
#define TRUECOLOR_BG_ORANGE   BG_TRUECOLOR(CODE_TRUECOLOR_ORANGE)
#define TRUECOLOR_BG_PURPLE   BG_TRUECOLOR(CODE_TRUECOLOR_PURPLE)
#define TRUECOLOR_BG_GREY     BG_TRUECOLOR(CODE_TRUECOLOR_GREY)

#define C256_WHITE    FG_TRUECOLOR(CODE_256_WHITE)
#define C256_GREEN    FG_TRUECOLOR(CODE_256_GREEN)
#define C256_RED      FG_TRUECOLOR(CODE_256_RED)
#define C256_BLUE     FG_TRUECOLOR(CODE_256_BLUE)
#define C256_DARKBLUE FG_TRUECOLOR(CODE_256_DARKBLUE)
#define C256_YELLOW   FG_TRUECOLOR(CODE_256_YELLOW)
#define C256_ORANGE   FG_TRUECOLOR(CODE_256_ORANGE)
#define C256_PURPLE   FG_TRUECOLOR(CODE_256_PURPLE)
#define C256_GREY     FG_TRUECOLOR(CODE_256_GREY)

#define C256_BG_WHITE    BG_TRUECOLOR(CODE_256_WHITE)
#define C256_BG_GREEN    BG_TRUECOLOR(CODE_256_GREEN)
#define C256_BG_RED      BG_TRUECOLOR(CODE_256_RED)
#define C256_BG_BLUE     BG_TRUECOLOR(CODE_256_BLUE)
#define C256_BG_DARKBLUE BG_TRUECOLOR(CODE_256_DARKBLUE)
#define C256_BG_YELLOW   BG_TRUECOLOR(CODE_256_YELLOW)
#define C256_BG_ORANGE   BG_TRUECOLOR(CODE_256_ORANGE)
#define C256_BG_PURPLE   BG_TRUECOLOR(CODE_256_PURPLE)
#define C256_BG_GREY     BG_TRUECOLOR(CODE_256_GREY)

namespace qat::cli {

const char* get_color(Color value) {
	auto cfg = Config::get();
	if (cfg->is_no_color_mode()) {
		return "";
	}
	switch (value) {
		case Color::white:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_WHITE : C256_WHITE;
		case Color::green:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_GREEN : C256_GREEN;
		case Color::red:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_RED : C256_RED;
		case Color::cyan:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_CYAN : C256_BLUE;
		case Color::blue:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_BLUE : C256_DARKBLUE;
		case Color::yellow:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_YELLOW : C256_YELLOW;
		case Color::orange:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_ORANGE : C256_ORANGE;
		case Color::purple:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_PURPLE : C256_PURPLE;
		case Color::grey:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_GREY : C256_GREY;
		case Color::reset:
			return colors::reset;
	}
}

const char* get_bg_color(Color value) {
	auto cfg = Config::get();
	if (cfg->is_no_color_mode()) {
		return "";
	}
	switch (value) {
		case Color::white:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_BG_WHITE : C256_BG_WHITE;
		case Color::green:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_BG_GREEN : C256_BG_GREEN;
		case Color::red:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_BG_RED : C256_BG_RED;
		case Color::cyan:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_BG_BLUE : C256_BG_BLUE;
		case Color::blue:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_BG_DARKBLUE : C256_BG_DARKBLUE;
		case Color::yellow:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_BG_YELLOW : C256_BG_YELLOW;
		case Color::orange:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_BG_ORANGE : C256_BG_ORANGE;
		case Color::purple:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_BG_PURPLE : C256_BG_PURPLE;
		case Color::grey:
			return cfg->color_mode() == ColorMode::truecolor ? TRUECOLOR_BG_GREY : C256_BG_GREY;
		case Color::reset:
			return colors::reset;
	}
}

} // namespace qat::cli