#ifndef HADES_GUI_HPP
#define HADES_GUI_HPP

#include <memory>
#include <string_view>
#include <unordered_map>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/RenderStates.hpp"
#include "SFML/Window/Event.hpp"

#include "hades/exceptions.hpp"
#include "hades/rectangle_math.hpp"
#include "hades/timers.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	struct colour;

	namespace resources
	{
		struct animation;
		struct animation_frame;
		struct font;
		struct texture;
	}

	namespace detail
	{
		template<typename T>
		constexpr bool gui_supported_types =
			std::is_same_v<T, int8> ||
			std::is_same_v<T, uint8> ||
			std::is_same_v<T, int16> ||
			std::is_same_v<T, uint16> ||
			std::is_same_v<T, int32> ||
			std::is_same_v<T, uint32> ||
			std::is_same_v<T, int64> ||
			std::is_same_v<T, uint64> ||
			std::is_same_v<T, float> ||
			std::is_same_v<T, double>;

		template<typename T>
		struct valid_input_scalar_t : std::false_type
		{};

		template<>
		struct valid_input_scalar_t<int> : std::true_type
		{};

		template<>
		struct valid_input_scalar_t<float> : std::true_type
		{};

		template<>
		struct valid_input_scalar_t<double> : std::true_type
		{};

		template<typename T>
		constexpr auto valid_input_scalar_v = valid_input_scalar_t<T>::value;

		using gui_context = ImGuiContext;
		struct gui_context_deleter
		{
			void operator()(gui_context*) const noexcept;
		};

		// concept for types that can be safely passed as dragdrop payloads
		template<typename T>
		concept DragDropPayload = std::is_trivially_copyable_v<T>;
	}

	class gui_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	// thrown if user tried to push a font that isn't part of the font atlas
	// create_fonts before new_frame is called
	class gui_font_missing : public gui_error
	{
	public:
		using gui_error::gui_error;
	};

	class gui_input_text_callback;

	template<detail::DragDropPayload T>
	class gui_dragdrop_payload;

	class gui : public sf::Drawable
	{
	public:
		gui(); 
		gui(std::string_view config_filename);

		gui(const gui&) = delete;
		gui(gui&&) noexcept = default;

		gui& operator=(const gui&) = delete;
		gui& operator=(gui&&) noexcept = default;

		using vector2 = vector2<float>;

		//should be called before using any gui function
		void activate_context() noexcept;

		//this must be called at least once to get valid output
		void set_display_size(vector2 size);

		//input must be inserted before frame_begin
		// returns true if the gui used the event
		//TODO: hades::event
		bool handle_event(const sf::Event& e);
		void update(time_duration dt);

		//copies the font into the gui system for use
		void create_font(const resources::font*);

		void frame_begin();
		void frame_end();

		void show_demo_window();
		void show_vertex_test_window();

		enum class window_flags : ImGuiWindowFlags {
			none = ImGuiWindowFlags_::ImGuiWindowFlags_None,
			no_titlebar = ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar,
			no_resize = ImGuiWindowFlags_::ImGuiWindowFlags_NoResize,
			no_move = ImGuiWindowFlags_::ImGuiWindowFlags_NoMove,
			no_scrollbar = ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar,
			no_scroll_with_mouse = ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollWithMouse,
			no_collapse = ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse,
			always_auto_resize = ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize,
			no_background = ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground,
			no_saved_settings = ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings,
			menubar = ImGuiWindowFlags_::ImGuiWindowFlags_MenuBar,
			horizontal_scrollbar = ImGuiWindowFlags_::ImGuiWindowFlags_HorizontalScrollbar,
			no_focus_on_appearing = ImGuiWindowFlags_::ImGuiWindowFlags_NoFocusOnAppearing,
			no_bring_to_front_on_focus = ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus,
			always_vertical_scrollbar = ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysVerticalScrollbar,
			always_horizontal_scrollbar = ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysHorizontalScrollbar,
			always_use_window_padding = ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysUseWindowPadding,
			no_nav_inputs = ImGuiWindowFlags_::ImGuiWindowFlags_NoNavInputs,
			no_nav_focus = ImGuiWindowFlags_::ImGuiWindowFlags_NoNavFocus,
			unsaved_document = ImGuiWindowFlags_::ImGuiWindowFlags_UnsavedDocument,
			no_nav = ImGuiWindowFlags_::ImGuiWindowFlags_NoNav,
			no_decoration = ImGuiWindowFlags_::ImGuiWindowFlags_NoDecoration,
			no_inputs = ImGuiWindowFlags_::ImGuiWindowFlags_NoInputs,
			panel = no_collapse |
			no_move |
			no_titlebar |
			no_resize |
			no_saved_settings
		};

		//windows and child windows
		//end must be called even if begin returns false
		bool window_begin(std::string_view name, bool& open, window_flags = window_flags::none);
		bool window_begin(std::string_view name, window_flags = window_flags::none);
		void window_end();

		//end must be called even if begin returns false
		bool child_window_begin(std::string_view name, vector2 size = { 0.f ,0.f }, bool border = false, window_flags = window_flags::none);
		void child_window_end();

		//window utilities
		//TODO: some funcs are missing
		bool is_window_appearing() const;
		vector2 window_position() const;
		vector2 window_size() const;

		// don't OR together like other flags
		enum class set_condition_enum : ImGuiCond
		{
			always = ImGuiCond_::ImGuiCond_Always,
			once = ImGuiCond_::ImGuiCond_Once, // Set the variable once per runtime session (only the first call will succeed)
			first_use = ImGuiCond_::ImGuiCond_FirstUseEver, // only triggers if their is no data in .ini file
			appearing = ImGuiCond_::ImGuiCond_Appearing // first recent use
		};

		//window manipulation
		//TODO: some funcs are missing
		void next_window_position(vector2);
		void next_window_size(vector2, set_condition_enum = set_condition_enum::always);

		// TODO: window scrolling
		//		some funcs missing
		float get_scroll_x() const;
		float get_scroll_y() const;
		void set_scroll_x(float scroll_x);
		void set_scroll_y(float scroll_y);
		float get_scroll_max_x() const;
		float get_scroll_max_y() const;
		void set_scroll_here_x(float);
		void set_scroll_here_y(float);

		enum class colour_target : ImGuiCol
		{
			text = ImGuiCol_::ImGuiCol_Text,
			text_disabled = ImGuiCol_::ImGuiCol_TextDisabled,
			window_background = ImGuiCol_::ImGuiCol_WindowBg,
			child_background = ImGuiCol_::ImGuiCol_ChildBg,
			popup_background = ImGuiCol_::ImGuiCol_PopupBg,
			border = ImGuiCol_::ImGuiCol_Border,
			border_shadow = ImGuiCol_::ImGuiCol_BorderShadow,
			frame_background = ImGuiCol_::ImGuiCol_FrameBg,
			frame_background_hover = ImGuiCol_::ImGuiCol_FrameBgHovered,
			frame_background_active = ImGuiCol_::ImGuiCol_FrameBgActive,
			title_background = ImGuiCol_::ImGuiCol_TitleBg,
			title_background_active = ImGuiCol_::ImGuiCol_TitleBgActive,
			title_background_collapsed = ImGuiCol_::ImGuiCol_TitleBgCollapsed,
			menu_bar_background = ImGuiCol_::ImGuiCol_MenuBarBg,
			scrollbar_background = ImGuiCol_::ImGuiCol_ScrollbarBg,
			scrollbar_grab = ImGuiCol_::ImGuiCol_ScrollbarGrab,
			scrollbar_grab_hovered = ImGuiCol_::ImGuiCol_ScrollbarGrabHovered,
			scrollbar_grab_active = ImGuiCol_::ImGuiCol_ScrollbarGrabActive,
			check_mark = ImGuiCol_::ImGuiCol_CheckMark,
			slider_grab = ImGuiCol_::ImGuiCol_SliderGrab,
			slider_grab_active = ImGuiCol_::ImGuiCol_SliderGrabActive,
			button = ImGuiCol_::ImGuiCol_Button,
			button_hovered = ImGuiCol_::ImGuiCol_ButtonHovered,
			button_active = ImGuiCol_::ImGuiCol_ButtonActive,
			header = ImGuiCol_::ImGuiCol_Header,
			header_hovered = ImGuiCol_::ImGuiCol_HeaderHovered,
			header_active = ImGuiCol_::ImGuiCol_HeaderActive,
			seperator = ImGuiCol_::ImGuiCol_Separator,
			seperator_hovered = ImGuiCol_::ImGuiCol_SeparatorHovered,
			seperator_active = ImGuiCol_::ImGuiCol_SeparatorActive,
			resize_grip = ImGuiCol_::ImGuiCol_ResizeGrip,
			resize_grip_hovered = ImGuiCol_::ImGuiCol_ResizeGripHovered,
			resize_grip_active = ImGuiCol_::ImGuiCol_ResizeGripActive,
			plot_lines = ImGuiCol_::ImGuiCol_PlotLines,
			plot_lines_hovered = ImGuiCol_::ImGuiCol_PlotLinesHovered,
			plot_histogram = ImGuiCol_::ImGuiCol_PlotHistogram,
			plot_histogram_hovered = ImGuiCol_::ImGuiCol_PlotHistogramHovered,
			text_selected_background = ImGuiCol_::ImGuiCol_TextSelectedBg,
			drag_drop_target = ImGuiCol_::ImGuiCol_DragDropTarget,
			navigation_highlight = ImGuiCol_::ImGuiCol_NavHighlight,
			navigation_window_highlight = ImGuiCol_::ImGuiCol_NavWindowingHighlight,
			navigation_window_dim_background = ImGuiCol_::ImGuiCol_NavWindowingDimBg,
			modal_window_dim_background = ImGuiCol_::ImGuiCol_ModalWindowDimBg,
		};

		//shared parameters
		// TODO: missing funcs
		void push_font(const resources::font*);
		void pop_font();
		void push_colour(colour_target element, const sf::Color&);
		void push_colour(colour_target element, const colour&);
		void pop_colour();

		//current window parameters
		// TODO: missing funcs
		void push_item_width(float width);
		void pop_item_width();
		void push_text_wrap_pos(float);
		void pop_text_wrap_pos();

		// TODO: style read (or not)

		//layouts
		// TODO: missing funcs
		void separator_horizontal();
		template<std::size_t Count = 1u>
		void indent();
		// TODO: deprecate, this is just an alias for same_line
		void layout_horizontal(float pos = 0.f, float width = -1.f);
		void same_line(float pos = 0.f, float width = -1.f);
		void same_line_wrapping(float next_item_size_x, float pos = 0.f, float width = -1.f); // sameline, unless the window width has been exhausted
		void layout_vertical(); //undoes layout_horizontal; newline
		void vertical_spacing();
		void group_begin();
		void group_end();
		void dummy(vector2_float size = {});
		float get_frame_height_with_spacing() const noexcept;

		//ids
		// push these onto the id stack to make later ids unique
		// ids are a hash of the entire stack
		void push_id(std::string_view);
		void push_id(int32);
		void push_id(const void*);
		void pop_id();

		//text widgets
		void text(std::string_view);
		void text_coloured(std::string_view, const sf::Color&);
		void text_coloured(std::string_view, const colour&);
		void text_disabled(std::string_view);
		void text_wrapped(std::string_view);
		// text_label
		void text_bullet(std::string_view);
		void separator_text(std::string_view); // seperator horizontal with title

		enum class direction : ImGuiDir
		{
			none = ImGuiDir_::ImGuiDir_None,
			left = ImGuiDir_::ImGuiDir_Left,
			right = ImGuiDir_::ImGuiDir_Right,
			up = ImGuiDir_::ImGuiDir_Up,
			down = ImGuiDir_::ImGuiDir_Down
		};

		//widgets
		bool button(std::string_view label, const vector2& size = { 0.f, 0.f });
		bool small_button(std::string_view label);
		bool invisible_button(std::string_view label, const vector2& size = { 0.f, 0.f });
		bool arrow_button(std::string_view label, direction);
		// TODO: use hades::colour
		void image(const resources::texture&, const rect_float& text_coords, const vector2& size, const sf::Color& tint_colour = sf::Color::White, const sf::Color& border_colour = sf::Color::Transparent);
		void image(const resources::animation&, const vector2 size, time_point time, const sf::Color& tint_colour = sf::Color::White, const sf::Color& border_colour = sf::Color::Transparent);
		void image(const resources::animation&, const vector2 size, int32 frame = {}, const sf::Color& tint_colour = sf::Color::White, const sf::Color& border_colour = sf::Color::Transparent);

		bool image_button(std::string_view id, const resources::texture&, const rect_float& text_coords, const vector2& size, const sf::Color& background_colour = sf::Color::Transparent, const sf::Color& tint_colour = sf::Color::White);
		bool image_button(std::string_view id, const resources::animation&, const vector2& size, time_point time = time_point{}, const sf::Color& background_colour = sf::Color::Transparent, const sf::Color& tint_colour = sf::Color::White);
		bool checkbox(std::string_view label, bool& checked); //returns true on checked changed
		//checkbox_flags
		bool radio_button(std::string_view label, bool active);
		template<typename T>
		bool radio_button(std::string_view label, T& active_selection, T this_button);
		void progress_bar(float progress, const vector2& size = { -1.f, 0.f });
		void progress_bar(float progress, std::string_view overlay_text, const vector2& size = { -1.f, 0.f });
		void bullet();

		// calculate the size of a button with the following settings
		vector2 calculate_button_size(std::string_view label, const vector2& size = {});

		enum class selectable_flag : ImGuiSelectableFlags
		{
			none = ImGuiSelectableFlags_::ImGuiSelectableFlags_None,
			dont_close_popups = ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups,
			span_all_columns = ImGuiSelectableFlags_::ImGuiSelectableFlags_SpanAllColumns, //TODO: tables
			allow_double_click = ImGuiSelectableFlags_::ImGuiSelectableFlags_AllowDoubleClick,
			disabled = ImGuiSelectableFlags_::ImGuiSelectableFlags_Disabled,
			allow_item_overlap = ImGuiSelectableFlags_::ImGuiSelectableFlags_AllowOverlap
		};

		//selectables
		//used as elements in comboboxes, etc
		bool selectable(std::string_view label, bool selected, selectable_flag = selectable_flag::none, const vector2& size = { 0.f, 0.f });
		bool selectable_easy(std::string_view label, bool& selected, selectable_flag = selectable_flag::none, const vector2& size = { 0.f, 0.f });

		enum class combo_flags : ImGuiComboFlags
		{
			none = ImGuiComboFlags_::ImGuiComboFlags_None,
			popup_align_left = ImGuiComboFlags_::ImGuiComboFlags_PopupAlignLeft,
			height_small = ImGuiComboFlags_::ImGuiComboFlags_HeightSmall,
			height_regular = ImGuiComboFlags_::ImGuiComboFlags_HeightRegular,
			height_large = ImGuiComboFlags_::ImGuiComboFlags_HeightLarge,
			height_largest = ImGuiComboFlags_::ImGuiComboFlags_HeightLargest,
			no_arrow_button = ImGuiComboFlags_::ImGuiComboFlags_NoArrowButton,
			no_preview = ImGuiComboFlags_::ImGuiComboFlags_NoPreview,
		};

		//combobox
		bool combo_begin(std::string_view label, std::string_view preview_value, combo_flags = combo_flags::none);
		//list selectables inbetween combo calls
		// only call combo_end if combo_begin returns true
		void combo_end();

		bool listbox_begin(std::string_view label, const vector2& size = {0.f, 0.f});
		// list selectables inbetween listbox calls
		void listbox_end(); // only call if listbox_begin returns true
		//quick listbox, handles the simple selectable listbox case
		//return true if selected item changes
		template<typename Container>
			requires string_type<typename Container::value_type>
		bool listbox(std::string_view label,
			std::size_t& current_item, const Container&, const vector2& size = { 0.f, 0.f });
		template<typename Container, string_transform<typename Container::value_type> ToString = nullptr_t>
		bool listbox(std::string_view label, std::size_t& current_item, const Container&,
			ToString&& = {}, const vector2& size = { 0.f, 0.f });


        /*template<typename Container, typename ToString = decltype(func_ref<string(typename Container::value_type)>(to_string))>
			requires string_type<std::invoke_result_t<ToString, typename Container::value_type>>
		bool listbox(std::string_view label, std::size_t& current_item,	const Container&,
            ToString = func_ref<string(typename Container::value_type)>(to_string),
			const vector2& size = { 0.f, 0.f });*/

		//TODO: drags

		enum class slider_flags : ImGuiSliderFlags
		{
			none = ImGuiSliderFlags_::ImGuiSliderFlags_None,
			always_clamp = ImGuiSliderFlags_AlwaysClamp, // clamp typed input as well
			logarithmic = ImGuiSliderFlags_Logarithmic,
			no_rounding = ImGuiSliderFlags_NoRoundToFormat, // dont round value to match input display(float)
			no_input = ImGuiSliderFlags_NoInput,       // disable typed input
		};

		// format can be used to decorate the value;
		// format follows printf rules
		bool slider_float(std::string_view label, float& v, float min, float max, slider_flags = slider_flags::none, std::string_view format = { "%.3f" });
		template<std::size_t N, std::enable_if_t< N < 5, int> = 0>
		bool slider_float(std::string_view label, std::array<float, N>& v, float min, float max, slider_flags = slider_flags::none, std::string_view format = { "%.3f" });
		bool slider_int(std::string_view label, int& v, int min, int max, slider_flags = slider_flags::none, std::string_view format = { "%d" });
		template<std::size_t N, std::enable_if_t< N < 5, int> = 0>
		bool slider_int(std::string_view label, std::array<int, N>& v, int min, int max, slider_flags = slider_flags::none, std::string_view format = { "%d" });

		template<typename T, typename U, std::enable_if_t<detail::gui_supported_types<T>, int> = 0>
		bool slider_scalar(std::string_view label, T &v, U min, U max, std::string_view = {}, slider_flags = slider_flags::none);
		//vslider = vertical sliders

		enum class input_text_flags : ImGuiInputTextFlags
		{
			none = ImGuiInputTextFlags_::ImGuiInputTextFlags_None,
			chars_decimal = ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsDecimal,
			chars_hexidecimal = ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsHexadecimal,
			chars_uppercase = ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsUppercase,
			chars_no_blank = ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsNoBlank,
			auto_select_all = ImGuiInputTextFlags_::ImGuiInputTextFlags_AutoSelectAll,
			enter_returns_true = ImGuiInputTextFlags_::ImGuiInputTextFlags_EnterReturnsTrue,
			callback_completion = ImGuiInputTextFlags_::ImGuiInputTextFlags_CallbackCompletion,
			callback_history = ImGuiInputTextFlags_::ImGuiInputTextFlags_CallbackHistory,
			callback_always = ImGuiInputTextFlags_::ImGuiInputTextFlags_CallbackAlways,
			callback_char_filter = ImGuiInputTextFlags_::ImGuiInputTextFlags_CallbackCharFilter,
			allow_tab_input = ImGuiInputTextFlags_::ImGuiInputTextFlags_AllowTabInput,
			ctrl_enter_for_newline = ImGuiInputTextFlags_::ImGuiInputTextFlags_CtrlEnterForNewLine,
			no_horizontal_scroll = ImGuiInputTextFlags_::ImGuiInputTextFlags_NoHorizontalScroll,
			always_overwrite = ImGuiInputTextFlags_::ImGuiInputTextFlags_AlwaysOverwrite,
			readonly = ImGuiInputTextFlags_::ImGuiInputTextFlags_ReadOnly,
			password = ImGuiInputTextFlags_::ImGuiInputTextFlags_Password,
			no_undo_redo = ImGuiInputTextFlags_::ImGuiInputTextFlags_NoUndoRedo,
			chars_scientific = ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsScientific,
			callback_resize = ImGuiInputTextFlags_::ImGuiInputTextFlags_CallbackResize,
			callback_edit = ImGuiInputTextFlags_::ImGuiInputTextFlags_CallbackEdit,
			escape_clears_all = ImGuiInputTextFlags_::ImGuiInputTextFlags_EscapeClearsAll
		};

		//inputs
		//returns true of the value of buffer changes
		// input calls the correct function from below, but doesn't support some 
		//	advanced options
		template<typename T> // selects the correct function from those bellow
		bool input(std::string_view label, T&, input_text_flags = input_text_flags::none);

		// text input
		// static sized input
		template<std::size_t Size, typename Callback = nullptr_t>
		bool input_text(std::string_view label, std::array<char, Size>& buffer, input_text_flags = input_text_flags::none, Callback = {});
		// dynamic sized input
		template<typename Callback = nullptr_t>
		bool input_text(std::string_view label, std::string& buffer, input_text_flags = input_text_flags::none, Callback = {});
		template<std::size_t Size>
		bool input_text_multiline(std::string_view label, std::array<char, Size> &buffer, const vector2 &size = { 0.f, 0.f }, input_text_flags = input_text_flags::none);
		bool input_text_multiline(std::string_view label, std::string &buffer, const vector2 &size = {0.f, 0.f}, input_text_flags = input_text_flags::none);
		
		// TODO: change step and step fast to U, as in slider_scalar above
		template<typename T, std::enable_if_t<detail::gui_supported_types<T>, int> = 0>
		bool input_scalar(std::string_view label, T &v, std::optional<T> step = static_cast<T>(1), std::optional<T> step_fast = static_cast<T>(1), input_text_flags = input_text_flags::none);
		template<typename T, std::size_t N, std::enable_if_t<detail::gui_supported_types<T>, int> = 0>
		bool input_scalar(std::string_view label, std::array<T, N>& v, std::optional<T> step = static_cast<T>(1), std::optional<T> step_fast = static_cast<T>(1), input_text_flags = input_text_flags::none);
		
		enum class colour_edit_flags : ImGuiColorEditFlags
		{
			none = ImGuiColorEditFlags_None,
			no_alpha = ImGuiColorEditFlags_NoAlpha,
			no_picker = ImGuiColorEditFlags_NoPicker,
			no_options = ImGuiColorEditFlags_NoOptions,
			no_preview = ImGuiColorEditFlags_NoSmallPreview,
			no_inputs = ImGuiColorEditFlags_NoInputs,
			no_tooltip = ImGuiColorEditFlags_NoTooltip,
			no_label = ImGuiColorEditFlags_NoLabel,
			no_side_preview = ImGuiColorEditFlags_NoSidePreview,
			no_drag_drop = ImGuiColorEditFlags_NoDragDrop,

			alpha_bar = ImGuiColorEditFlags_AlphaBar,
			alpha_preview = ImGuiColorEditFlags_AlphaPreview,
			alpha_preview_half = ImGuiColorEditFlags_AlphaPreviewHalf,
			format_hdr = ImGuiColorEditFlags_HDR,
			format_rgb = ImGuiColorEditFlags_DisplayRGB,
			format_hsv = ImGuiColorEditFlags_DisplayHSV,
			format_hex = ImGuiColorEditFlags_DisplayHex,
			format_uint8 = ImGuiColorEditFlags_Uint8,
			format_float = ImGuiColorEditFlags_Float,
			hue_bar = ImGuiColorEditFlags_PickerHueBar,
			hue_wheel = ImGuiColorEditFlags_PickerHueWheel,

			restore_default = ImGuiColorEditFlags_DefaultOptions_
		};

		//3/4, 3 = rgb, 4 = rbga
		//edit creates a preview square the summons a picker dialog
		//picker creates a picker widget
		void colour_editor_options(colour_edit_flags);
		bool colour_edit(std::string_view label, colour& colour, colour_edit_flags = colour_edit_flags::none);
		bool colour_picker(std::string_view label, std::array<uint8, 3> &colour, colour_edit_flags = colour_edit_flags::none);
		bool colour_picker(std::string_view label, std::array<uint8, 4> &colour, colour_edit_flags = colour_edit_flags::none);
		bool colour_picker(std::string_view label, colour& colour, colour_edit_flags = colour_edit_flags::none);

		//TODO: update
		enum class tree_node_flags : ImGuiTreeNodeFlags
		{
			none = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_None,
			selected = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected,
			framed = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Framed,
			allow_item_overlap = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_AllowOverlap,
			no_tree_push_on_open = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_NoTreePushOnOpen,
			no_auto_open_on_log = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_NoAutoOpenOnLog,
			default_open = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_DefaultOpen,
			open_on_double_click = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_OpenOnDoubleClick,
			open_on_arrow = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_OpenOnArrow,
			leaf = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Leaf,
			bullet = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Bullet,
			frame_padding = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_FramePadding,
			nav_left_jumps_back_here = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_NavLeftJumpsBackHere
		};

		//TODO: Tree
		bool tree_node(std::string_view, tree_node_flags = tree_node_flags::none);
		void tree_pop(); //  call if tree_node returns true
		bool collapsing_header(std::string_view, tree_node_flags = tree_node_flags::none);
		bool collapsing_header(std::string_view, bool &open, tree_node_flags = tree_node_flags::none); 
		// set whether the next tree_node or collapsing_header should be open
		void set_next_item_open(bool is_open, set_condition_enum = set_condition_enum::always);

		//TODO: list box(see above)

		//TODO: other widgets

		//TODO: plotting

		//TODO: value


		bool main_menubar_begin();
		// Only call if main_menubar_begin() returns true
		void main_menubar_end();
		bool menubar_begin();
		void menubar_end();
		// TODO: remove the enabled params, users need to use begin_disabled/end_disabled
		bool menu_begin(std::string_view, bool enabled = true);
		// Only call if menu_begin() returns true 
		void menu_end();
		bool menu_item(std::string_view, bool enabled = true);
		bool menu_toggle_item(std::string_view, bool &selected, bool enabled = true);

		bool main_toolbar_begin();
		void main_toolbar_end();
		bool toolbar_button(std::string_view);
		bool toolbar_button(std::string_view, const resources::animation&);
		void toolbar_separator();

		//tooltips
		//begin/end tooltip

		[[nodiscard]]
		bool tooltip_begin(); // only if returns true, must call tooltip_end
		void tooltip_end();
		//equivelent to checking is_item_hovered and calling begin/end tooltip with a text element
		void tooltip(std::string_view); 
	
		//TODO: popups
		//popup windows are defined by the popup_begin/popup_end functions
		//they are flagged as open by calling open_popup elsewhere in the program
		//they are otherwise normal windows, though they will close on any input
		//not directed at them
		void open_popup(std::string_view);
		//TODO: open_popup on stored id rather than string
		void popup_begin(std::string_view);
		//only call if popup_begin returned true
		void popup_end();
		void close_current_popup();

		//TODO: popup context memnus
		//		popup void?
		//		popup window

		//modal dialogs(block input behind them)
		//same as popups above, but input not directed at them is just ignored
		void open_modal(std::string_view);
		bool modal_begin(std::string_view, window_flags flags = window_flags::none);
		//only call if modal_begin returned true
		void modal_end();
		void close_current_modal();

		enum class sort_direction : ImGuiSortDirection
		{
			none = ImGuiSortDirection_None,
			ascending =ImGuiSortDirection_Ascending,    // Ascending = 0->9, A->Z etc.
			descending = ImGuiSortDirection_Descending	// Descending = 9->0, Z->A etc.
		};

		enum class table_flags : ImGuiTableFlags
		{
			// Features
			none = ImGuiTableFlags_None,
			resizeable = ImGuiTableFlags_Resizable,   // Enable resizing columns.
			reorderable = ImGuiTableFlags_Reorderable,   // Enable reordering columns in header row (need calling TableSetupColumn() + TableHeadersRow() to display headers)
			hidable = ImGuiTableFlags_Hideable,   // Enable hiding/disabling columns in context menu.
			sortable = ImGuiTableFlags_Sortable,   // Enable sorting. Call TableGetSortSpecs() to obtain sort specs. Also see ImGuiTableFlags_SortMulti and ImGuiTableFlags_SortTristate.
			no_saved_settings = ImGuiTableFlags_NoSavedSettings,   // Disable persisting columns order, width and sort settings in the .ini file.
			context_menu_in_body = ImGuiTableFlags_ContextMenuInBody,   // Right-click on columns body/contents will display table context menu. By default it is available in TableHeadersRow().
			// Decorations
			row_bg = ImGuiTableFlags_RowBg,   // Set each RowBg color with ImGuiCol_TableRowBg or ImGuiCol_TableRowBgAlt (equivalent of calling TableSetBgColor with ImGuiTableBgFlags_RowBg0 on each row manually)
			borders_inner_h = ImGuiTableFlags_BordersInnerH,   // Draw horizontal borders between rows.
			borders_outer_h = ImGuiTableFlags_BordersOuterH,   // Draw horizontal borders at the top and bottom.
			borders_inner_v = ImGuiTableFlags_BordersInnerV,   // Draw vertical borders between columns.
			borders_outer_v = ImGuiTableFlags_BordersOuterV,  // Draw vertical borders on the left and right sides.
			borders_h = ImGuiTableFlags_BordersH, // Draw horizontal borders.
			borders_v = ImGuiTableFlags_BordersV, // Draw vertical borders.
			borders_inner = ImGuiTableFlags_BordersInner, // Draw inner borders.
			borders_outer = ImGuiTableFlags_BordersOuter, // Draw outer borders.
			borders = ImGuiTableFlags_Borders,   // Draw all borders.
			no_borders_in_body = ImGuiTableFlags_NoBordersInBody,  // [ALPHA] Disable vertical borders in columns Body (borders will always appears in Headers). -> May move to style
			no_borders_in_body_until_resize = ImGuiTableFlags_NoBordersInBodyUntilResize,  // [ALPHA] Disable vertical borders in columns Body until hovered for resize (borders will always appears in Headers). -> May move to style
			// Sizing Policy (read above for defaults)
			sizing_fixed_fit = ImGuiTableFlags_SizingFixedFit,  // Columns default to _WidthFixed or _WidthAuto (if resizable or not resizable), matching contents width.
			sizing_fixed_same = ImGuiTableFlags_SizingFixedSame,  // Columns default to _WidthFixed or _WidthAuto (if resizable or not resizable), matching the maximum contents width of all columns. Implicitly enable ImGuiTableFlags_NoKeepColumnsVisible.
			sizing_stretch_prop = ImGuiTableFlags_SizingStretchProp,  // Columns default to _WidthStretch with default weights proportional to each columns contents widths.
			sizing_stretch_same = ImGuiTableFlags_SizingStretchSame,  // Columns default to _WidthStretch with default weights all equal, unless overridden by TableSetupColumn().
			// Sizing Extra Options
			no_host_extend_x = ImGuiTableFlags_NoHostExtendX,  // Make outer width auto-fit to columns, overriding outer_size.x value. Only available when ScrollX/ScrollY are disabled and Stretch columns are not used.
			no_host_extend_y = ImGuiTableFlags_NoHostExtendY,  // Make outer height stop exactly at outer_size.y (prevent auto-extending table past the limit). Only available when ScrollX/ScrollY are disabled. Data below the limit will be clipped and not visible.
			no_keep_columns_visible = ImGuiTableFlags_NoKeepColumnsVisible,  // Disable keeping column always minimally visible when ScrollX is off and table gets too small. Not recommended if columns are resizable.
			precise_width = ImGuiTableFlags_PreciseWidths,  // Disable distributing remainder width to stretched columns (width allocation on a 100-wide table with 3 columns: Without this flag: 33,33,34. With this flag: 33,33,33). With larger number of columns, resizing will appear to be less smooth.
			// Clipping
			no_clip = ImGuiTableFlags_NoClip,  // Disable clipping rectangle for every individual columns (reduce draw command count, items will be able to overflow into other columns). Generally incompatible with TableSetupScrollFreeze().
			// Padding
			pad_outer_x = ImGuiTableFlags_PadOuterX,  // Default if BordersOuterV is on. Enable outer-most padding. Generally desirable if you have headers.
			no_pad_outer_x = ImGuiTableFlags_NoPadOuterX,  // Default if BordersOuterV is off. Disable outer-most padding.
			no_pad_inner_x = ImGuiTableFlags_NoPadInnerX,  // Disable inner padding between columns (double inner padding if BordersOuterV is on, single inner padding if BordersOuterV is off).
			// Scrolling
			scroll_x = ImGuiTableFlags_ScrollX,  // Enable horizontal scrolling. Require 'outer_size' parameter of BeginTable() to specify the container size. Changes default sizing policy. Because this create a child window, ScrollY is currently generally recommended when using ScrollX.
			scroll_y = ImGuiTableFlags_ScrollY,  // Enable vertical scrolling. Require 'outer_size' parameter of BeginTable() to specify the container size.
			// Sorting
			sort_multi = ImGuiTableFlags_SortMulti,  // Hold shift when clicking headers to sort on multiple column. TableGetSortSpecs() may return specs where (SpecsCount > 1).
			sort_tristate = ImGuiTableFlags_SortTristate
		};

		enum class table_row_flags : ImGuiTableRowFlags
		{
			none = ImGuiTableRowFlags_None,
			headers = ImGuiTableRowFlags_Headers    // Identify header row (set default background color + width of its contents accounted differently for auto column width)
		};

		enum class table_column_flags : ImGuiTableColumnFlags
		{
			// Input configuration flags
			none = ImGuiTableColumnFlags_None,
			disabled = ImGuiTableColumnFlags_Disabled,   // Overriding/master disable flag: hide column, won't show in context menu (unlike calling TableSetColumnEnabled() which manipulates the user accessible state)
			default_hide = ImGuiTableColumnFlags_DefaultHide,   // Default as a hidden/disabled column.
			default_sort = ImGuiTableColumnFlags_DefaultSort,   // Default as a sorting column.
			width_stretch = ImGuiTableColumnFlags_WidthStretch,   // Column will stretch. Preferable with horizontal scrolling disabled (default if table sizing policy is _SizingStretchSame or _SizingStretchProp).
			width_fixed = ImGuiTableColumnFlags_WidthFixed,   // Column will not stretch. Preferable with horizontal scrolling enabled (default if table sizing policy is _SizingFixedFit and table is resizable).
			no_resize = ImGuiTableColumnFlags_NoResize,   // Disable manual resizing.
			no_reorder = ImGuiTableColumnFlags_NoReorder,   // Disable manual reordering this column, this will also prevent other columns from crossing over this column.
			no_hide = ImGuiTableColumnFlags_NoHide,   // Disable ability to hide/disable this column.
			no_clip = ImGuiTableColumnFlags_NoClip,   // Disable clipping for this column (all NoClip columns will render in a same draw command).
			no_sort = ImGuiTableColumnFlags_NoSort,   // Disable ability to sort on this field (even if ImGuiTableFlags_Sortable is set on the table).
			no_sort_ascending = ImGuiTableColumnFlags_NoSortAscending,  // Disable ability to sort in the ascending direction.
			no_sort_descending = ImGuiTableColumnFlags_NoSortDescending,  // Disable ability to sort in the descending direction.
			no_header_label = ImGuiTableColumnFlags_NoHeaderLabel,  // TableHeadersRow() will not submit label for this column. Convenient for some small columns. Name will still appear in context menu.
			no_header_width = ImGuiTableColumnFlags_NoHeaderWidth,  // Disable header text width contribution to automatic column width.
			prefer_sort_ascending = ImGuiTableColumnFlags_PreferSortAscending,  // Make the initial sort direction Ascending when first sorting on this column (default).
			prefer_sort_descending = ImGuiTableColumnFlags_PreferSortDescending,  // Make the initial sort direction Descending when first sorting on this column.
			indent_enabled = ImGuiTableColumnFlags_IndentEnable,  // Use current Indent value when entering cell (default for column 0).
			indent_disabled = ImGuiTableColumnFlags_IndentDisable,  // Ignore current Indent value when entering cell (default for columns > 0). Indentation changes _within_ the cell will still be honored.		
		};

		// Output status flags, read-only via TableGetColumnFlags()
		enum class table_column_status : ImGuiTableColumnFlags
		{
			enabled = ImGuiTableColumnFlags_IsEnabled,  // Status: is enabled == not hidden by user/api (referred to as "Hide" in _DefaultHide and _NoHide) flags.
			visible = ImGuiTableColumnFlags_IsVisible,  // Status: is visible == is enabled AND not clipped by scrolling.
			sorted = ImGuiTableColumnFlags_IsSorted,	// Status: is currently part of the sort specs
			hovered = ImGuiTableColumnFlags_IsHovered,  // Status: is hovered by mouse
		};

		// tables
		// NOTE: must call table_next_column for the first column
		bool begin_table(std::string_view id, int column_count,
			table_flags flags = table_flags::none, const vector2& outer_size = {}, float inner_width = {});
		// only call if begin_table returned true
		void end_table();
		void table_setup_column(std::string_view label, table_column_flags = table_column_flags::none,
			float init_width_or_weight = 0.0f/*, ImGuiID user_id = 0*/);

		void table_headers_row(); // automatically setup headers as defined in table_setup_column
		// start a new row, must call table_next_column or table_set_column_index to get the correct column
		void table_next_row(table_row_flags row_flags = table_row_flags::none, float min_row_height = {}); // append into the first cell of a new row.
		bool table_next_column();                                  // append into the next column (or first column of next row if currently in last column). Return true when column is visible.
		bool table_set_column_index(int column_n);

		// if true then you need to call table_sort_column
		bool table_needs_sort() const noexcept;
		
		struct sort_column_return
		{
			int column;
			sort_direction direction;
		};
		
		// call with increasing sort_order until sort_direction returns none
		sort_column_return table_sort_column(int sort_order) const noexcept;

		//columns, soon to be deprecated
		void columns_begin(std::size_t count = 1u, bool border = true);
		void columns_begin(std::string_view id, std::size_t count = 1u, bool border = true);
		void columns_next();
		void columns_end();

		//TODO: tabbars

		// Drag Drop
		enum class dragdrop_flags : ImGuiDragDropFlags
		{
			none = ImGuiDragDropFlags_None,
			// BeginDragDropSource() flags
			// By default, a successful call to BeginDragDropSource opens a tooltip so 
			// you can display a preview or description of the source contents. This flag disables this behavior.
			source_no_preview_tooltip = ImGuiDragDropFlags_SourceNoPreviewTooltip, 
			// By default, when dragging we clear data so that IsItemHovered() will return false,
			// to avoid subsequent user code submitting tooltips. This flag disables this behavior
			// so you can still call IsItemHovered() on the source item.
			source_no_disable_hover = ImGuiDragDropFlags_SourceNoDisableHover,   
			// Disable the behavior that allows to open tree nodes and collapsing header by holding
			// over them while dragging a source item.
			source_no_hold_to_open_others = ImGuiDragDropFlags_SourceNoHoldToOpenOthers,
			// Allow items such as Text(), Image() that have no unique identifier to be used as drag source,
			// by manufacturing a temporary identifier based on their window-relative position. 
			// This is extremely unusual within the dear imgui ecosystem and so we made it explicit.
			source_allow_null_id = ImGuiDragDropFlags_SourceAllowNullID,
			// External source (from outside of dear imgui), won't attempt to read current item/window info.
			// Will always return true. Only one Extern source can be active simultaneously.
			source_extern = ImGuiDragDropFlags_SourceExtern,   
			// Automatically expire the payload if the source cease to be submitted
			// (otherwise payloads are persisting while being dragged)
			source_auto_expire_payload = ImGuiDragDropFlags_SourceAutoExpirePayload,   
			// AcceptDragDropPayload() flags
			// AcceptDragDropPayload() will returns true even before the mouse button is released.
			// You can then call IsDelivery() on the return value to test if the payload needs to be delivered.
			accept_before_delivery = ImGuiDragDropFlags_AcceptBeforeDelivery,  
			// Do not draw the default highlight rectangle when hovering over target.
			accept_no_draw_default_highlight = ImGuiDragDropFlags_AcceptNoDrawDefaultRect, 
			// Request hiding the BeginDragDropSource tooltip from the BeginDragDropTarget site.
			accept_no_preview_tooltip = ImGuiDragDropFlags_AcceptNoPreviewTooltip,  
			// For peeking ahead and inspecting the payload before delivery.
			accept_peek_only = ImGuiDragDropFlags_AcceptPeekOnly  
		};

		// call after submitting an item which may be dragged. when this returns true, you can call SetDragDropPayload() + EndDragDropSource()
		bool begin_dragdrop_source(dragdrop_flags = dragdrop_flags::none);
		// Data is copied and held by imgui. Return true when payload has been accepted.
		bool set_dragdrop_payload(detail::DragDropPayload auto payload, std::string_view type_name, set_condition_enum = set_condition_enum::always);
		// only call EndDragDropSource() if BeginDragDropSource() returns true!
		void end_dragdrop_source();
		// call after submitting an item that may receive a payload. If this returns true, you can call AcceptDragDropPayload() + EndDragDropTarget()
		bool begin_dragdrop_target();
		// accept contents of a given type. If ImGuiDragDropFlags_AcceptBeforeDelivery is set you can peek into the payload before the mouse button is released.
		template<detail::DragDropPayload T>
		gui_dragdrop_payload<T> accept_dragdrop_payload(std::string_view type_name, dragdrop_flags = dragdrop_flags::none);
		void end_dragdrop_target();

		// Disabled 
		void begin_disabled();
		void end_disabled();

		// TODO: clip rect?

		// Focus
		// offset targets the item to be focused
		//	-1: prev item	//other negative numbers are forbidden
		//	0: next item
		//	1+: item after next, etc
		void set_keyboard_focus_here(int offset = 0) noexcept; 
		// sets previous item to be the windows default focus
		void set_item_default_focus() noexcept;

		enum class hovered_flags : ImGuiHoveredFlags
		{
			none = ImGuiHoveredFlags_::ImGuiHoveredFlags_None,
			child_windows = ImGuiHoveredFlags_::ImGuiHoveredFlags_ChildWindows,
			root_window = ImGuiHoveredFlags_::ImGuiHoveredFlags_RootWindow,
			any_window = ImGuiHoveredFlags_::ImGuiHoveredFlags_AnyWindow,
			allow_when_blocked_by_popup = ImGuiHoveredFlags_::ImGuiHoveredFlags_AllowWhenBlockedByPopup,
			allow_when_blocked_by_active_item = ImGuiHoveredFlags_::ImGuiHoveredFlags_AllowWhenBlockedByActiveItem,
			allow_when_overlapped = ImGuiHoveredFlags_::ImGuiHoveredFlags_AllowWhenOverlapped,
			allow_when_disabled = ImGuiHoveredFlags_::ImGuiHoveredFlags_AllowWhenDisabled,
			rect_only = ImGuiHoveredFlags_::ImGuiHoveredFlags_RectOnly,
			root_and_child_windows = ImGuiHoveredFlags_::ImGuiHoveredFlags_RootAndChildWindows,
			for_tooltips = ImGuiHoveredFlags_::ImGuiHoveredFlags_ForTooltip,

		};

		enum class mouse_button : ImGuiMouseButton
		{
			left = ImGuiMouseButton_::ImGuiMouseButton_Left,
			right = ImGuiMouseButton_::ImGuiMouseButton_Right,
			middle =ImGuiMouseButton_::ImGuiMouseButton_Middle,
			count = ImGuiMouseButton_::ImGuiMouseButton_COUNT
		};

		// input state
		vector2 get_mouse_pos() const;

		//item utils
		bool is_item_hovered(hovered_flags = hovered_flags::none); //returns true if mouse is over object
		bool is_item_focused(); //returns true if item has keyboard or gamepad focusIsItemActive();                                                     // is the last item active? (e.g. button being held, text field being edited. This will continuously return true while holding mouse button on an item. Items that don't interact will always return false)
		bool is_item_clicked(mouse_button = mouse_button::left);
		bool is_item_edited();    
		bool is_item_toggled_open();
		vector2 get_item_rect_min();
		vector2 get_item_rect_max();
		vector2 get_item_rect_size();

		//utils
		vector2 calculate_text_size(std::string_view, bool include_text_after_double_hash = true, float wrap_width = -1.0f);

		//mouse utils(cursor and so on)

		void draw(sf::RenderTarget& target, const sf::RenderStates& states = sf::RenderStates{}) const override;

		static constexpr std::string_view version() noexcept
		{
			return IMGUI_VERSION;
		}

	private:
		using font = ImFont;

		void _activate_context() noexcept;
		void _active_assert() const noexcept;
		static font* _get_font(const resources::font*);
		static void _create_font(const resources::font*);
		static void _create_default_font();
		static void _generate_atlas();
		void _image_animation_frame(const resources::animation&, const vector2 size,
			const resources::animation_frame&, const sf::Color& tint_colour,
			const sf::Color& border_colour);

		std::string _ini_filename; // Placed above context so that it is
								   // still valid during contexts destructor.
		using context_ptr = std::unique_ptr<detail::gui_context, detail::gui_context_deleter>;
		context_ptr _my_context;

		// used to help place the toolbar window in the correct location
		struct toolbar_layout_info
		{
			float main_menubar_y2;
			float width;
		};

		toolbar_layout_info _main_toolbar_info;

		//static font atlas will be shared for all instances of gui
		static std::unique_ptr<ImFontAtlas> _font_atlas;
		static std::unordered_map<const resources::font*, ImFont*> _fonts;
	};

	class gui_input_text_callback
	{
	public:
		explicit gui_input_text_callback(ImGuiInputTextCallbackData*) noexcept;
		// the flag that triggered the callback
		// one of:
		//	callback_completion
		//	callback_history
		//	callback_always 
		//	callback_char_filter
		//	callback_resize 
		//	callback_edit
		gui::input_text_flags get_callback_trigger() const noexcept; 
		gui::input_text_flags get_input_flags() const noexcept;
		
		//support for char filter flag
		ImWchar get_char_filter_input() const noexcept;
		// set to '0' to deny the input.
		void set_char_filter_replacement(ImWchar) noexcept;

		enum class input_key : std::underlying_type_t<ImGuiKey> {
			tab = ImGuiKey_Tab,
			up = ImGuiKey_UpArrow,
			down = ImGuiKey_DownArrow
		};

		// input key to trigger autocomplete/history flags
		input_key get_input_key() const noexcept;
		// current input buffer contents
		std::string_view input_contents() const noexcept;
		int cursor_pos() const noexcept;

		void set_cursor_pos(int) noexcept;

		bool has_selection() const noexcept;
		std::pair<int, int> selection_range() const noexcept;
		void set_selection_range(int, int) noexcept;
		
		void erase_chars(int pos, int count) noexcept;
		void clear_chars() noexcept;
		void insert_chars(int pos, std::string_view text);
		void select_all() noexcept;
		void clear_selection() noexcept;

	private:
		ImGuiInputTextCallbackData* _data;
	};

	template<detail::DragDropPayload T>
	class gui_dragdrop_payload
	{
	public:
		gui_dragdrop_payload(const ImGuiPayload* p)
			: _payload{ p } {}

		bool is_correct_type(std::string_view type) const noexcept
		{
			return _payload && _payload->IsDataType(to_string(type).c_str());
		}

		bool is_preview() const noexcept
		{
			return _payload && _payload->IsPreview();
		}

		bool is_delivery() const noexcept
		{
			return _payload && _payload->IsDelivery();
		}

		T get() const noexcept
		{
			assert(_payload && _payload->Data);
			assert(sizeof(T) == _payload->DataSize);
			return *static_cast<T*>(_payload->Data);
		}

	private:
		const ImGuiPayload* _payload;
	};

	namespace detail
	{
		template<typename Enum>
		concept EnumFlag = requires(Enum a, Enum b)
		{
			requires std::is_enum_v<Enum>;
			a | b;
		};

		template<typename Enum>
		constexpr inline Enum enum_or(Enum lhs, Enum rhs) noexcept
		{
			using T = std::underlying_type_t<Enum>;
			return static_cast<Enum>(static_cast<T>(lhs) | static_cast<T>(rhs));
		}
	}

	constexpr inline gui::window_flags operator|(gui::window_flags lhs, gui::window_flags rhs) noexcept
	{
		return detail::enum_or(lhs, rhs);
	}

	//TODO: selectable, combo, etc...
	constexpr inline gui::colour_edit_flags operator|(gui::colour_edit_flags lhs, gui::colour_edit_flags rhs) noexcept
	{
		return detail::enum_or(lhs, rhs);
	}

	constexpr inline gui::input_text_flags operator|(gui::input_text_flags lhs, gui::input_text_flags rhs) noexcept
	{
		return detail::enum_or(lhs, rhs);
	}

	constexpr inline gui::tree_node_flags operator|(gui::tree_node_flags lhs, gui::tree_node_flags rhs) noexcept
	{
		return detail::enum_or(lhs, rhs);
	}

	constexpr inline gui::table_flags operator|(gui::table_flags lhs, gui::table_flags rhs) noexcept
	{
		return detail::enum_or(lhs, rhs);
	}

	constexpr inline gui::table_column_flags operator|(gui::table_column_flags lhs, gui::table_column_flags rhs) noexcept
	{
		return detail::enum_or(lhs, rhs);
	}	

	constexpr inline gui::dragdrop_flags operator|(gui::dragdrop_flags lhs, gui::dragdrop_flags rhs) noexcept
	{
		return detail::enum_or(lhs, rhs);
	}

	constexpr inline detail::EnumFlag auto operator|=(detail::EnumFlag auto& a, const detail::EnumFlag auto b) noexcept
	{
		return a = a | b;
	}
}

#include "hades/detail/gui.inl"

#endif //!HADES_GUI_HPP
