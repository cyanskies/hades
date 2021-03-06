#ifndef HADES_GUI_HPP
#define HADES_GUI_HPP

#include <memory>
#include <string_view>
#include <unordered_map>

#include "imgui.h"

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Window/Event.hpp"

#include "hades/rectangle_math.hpp"
#include "hades/timers.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	namespace resources
	{
		struct animation;
		struct font;
		struct texture;
	}

	namespace detail
	{
		//we can throw a container holding a string straight into the listbox
		//NOTE: we cannot use a string_view without processing
		template<typename Cont>
		using listbox_with_string = std::enable_if_t<
			is_string_v<typename Cont::value_type> &&
			!std::is_same_v<std::string_view, typename Cont::value_type>,
			bool>;

		//a string_view or anything other than a string, we generate a default
		// to_string function
		template<typename Cont>
		using listbox_no_string = std::enable_if_t<
			!is_string_v<typename Cont::value_type> ||
			std::is_same_v<std::string_view, typename Cont::value_type>,
			bool>;

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
	}

	class gui : public sf::Drawable
	{
	public:
		gui();

		gui(const gui&) = delete;
		gui(gui&&) noexcept = default;

		gui &operator=(const gui&) = delete;
		gui &operator=(gui&&) noexcept = default;

		//should be called before using any gui function
		void activate_context() noexcept;

		//this must be called at least once to get valid output
		void set_display_size(vector_t<float> size);
		
		//input must be inserted before frame_begin
		// returns true if the gui used the event
		//TODO: hades::event
		bool handle_event(const sf::Event &e);
		void update(time_duration dt);

		void frame_begin();
		void frame_end();

		void show_demo_window();

		enum class window_flags : ImGuiWindowFlags {
			none = ImGuiWindowFlags_::ImGuiWindowFlags_None,
			no_titlebar = ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar,
			no_resize = ImGuiWindowFlags_::ImGuiWindowFlags_NoResize,
			no_move = ImGuiWindowFlags_::ImGuiWindowFlags_NoMove,
			no_scrollbar = ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar,
			no_scroll_with_mouse = ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollWithMouse,
			no_collapse = ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse,
			always_auto_resize = ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize,
			no_saved_settings = ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings,
			no_inputs = ImGuiWindowFlags_::ImGuiWindowFlags_NoInputs,
			menubar = ImGuiWindowFlags_::ImGuiWindowFlags_MenuBar,
			horizontal_scrollbar = ImGuiWindowFlags_::ImGuiWindowFlags_HorizontalScrollbar,
			no_focus_on_appearing = ImGuiWindowFlags_::ImGuiWindowFlags_NoFocusOnAppearing,
			no_bring_to_front_on_focus = ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus,
			always_vertical_scrollbar = ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysVerticalScrollbar,
			always_horizontal_scrollbar = ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysHorizontalScrollbar,
			always_use_window_padding = ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysUseWindowPadding,
			no_nav_inputs = ImGuiWindowFlags_::ImGuiWindowFlags_NoNavInputs,
			no_nav_focus = ImGuiWindowFlags_::ImGuiWindowFlags_NoNavFocus,
			no_nav = ImGuiWindowFlags_::ImGuiWindowFlags_NoNav,
			panel =	no_collapse |
					no_move |
					no_titlebar |
					no_resize |
					no_saved_settings
		};

		//windows and child windows
		//end must be called even if begin returns false
		bool window_begin(std::string_view name, bool &closed, window_flags = window_flags::none);
		bool window_begin(std::string_view name, window_flags = window_flags::none);
		void window_end();

		using vector = vector_t<float>;
		//end must be called even if begin returns false
		bool child_window_begin(std::string_view name, vector size = { 0.f ,0.f }, bool border = false, window_flags = window_flags::none);
		void child_window_end();

		//window utilities
		vector window_position() const;
		vector window_size() const;
		void next_window_position(vector);
		void next_window_size(vector);

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
		void push_font(const resources::font*);
		void pop_font();
		void push_colour(colour_target element, const sf::Color&);
		void pop_colour();

		//current window parameters
		void push_item_width(float width);
		void pop_item_width();

		//layouts
		void separator_horizontal();
		template<std::size_t Count = 1u>
		void indent();
		void layout_horizontal(float pos = 0.f, float width = -1.f);
		void layout_vertical(); //undoes layout_horizontal
		void vertical_spacing();
		void group_begin();
		void group_end();
		void dummy(vector_float size = {});

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
		void text_disabled(std::string_view);
		void text_bullet(std::string_view);
		void text_wrapped(std::string_view);
		
		enum class direction : ImGuiDir 
		{
			none = ImGuiDir_::ImGuiDir_None,
			left = ImGuiDir_::ImGuiDir_Left,
			right = ImGuiDir_::ImGuiDir_Right,
			up = ImGuiDir_::ImGuiDir_Up,
			down = ImGuiDir_::ImGuiDir_Down
		};

		//widgets
		bool button(std::string_view label, const vector &size = {0.f, 0.f});
		bool small_button(std::string_view label);
		bool invisible_button(std::string_view label, const vector &size = { 0.f, 0.f });
		bool arrow_button(std::string_view label, direction);
		void image(const resources::texture&, const rect_float &text_coords, const vector &size, const sf::Color &tint_colour = sf::Color::White, const sf::Color &border_colour = sf::Color::Transparent);
		void image(const resources::animation&, const vector &size, time_point time = time_point{}, const sf::Color &tint_colour = sf::Color::White, const sf::Color &border_colour = sf::Color::Transparent);
		bool image_button(const resources::texture&, const rect_float &text_coords, const vector &size, const sf::Color &background_colour = sf::Color::Transparent, const sf::Color &tint_colour = sf::Color::White);
		bool image_button(const resources::animation&, const vector &size, time_point time = time_point{}, const sf::Color &background_colour = sf::Color::Transparent, const sf::Color &tint_colour = sf::Color::White);
		bool checkbox(std::string_view label, bool &checked); //returns true on checked changed
		bool radio_button(std::string_view label, bool active);
		template<typename T>
		bool radio_button(std::string_view label, T &active_selection, T this_button);
		void progress_bar(float progress, const vector &size = { -1.f, 0.f });
		void progress_bar(float progress, std::string_view overlay_text, const vector &size = { -1.f, 0.f });
		void bullet();

		enum class selectable_flag : ImGuiSelectableFlags
		{
			none = ImGuiSelectableFlags_::ImGuiSelectableFlags_None,
			dont_close_popups = ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups,
			span_all_columns = ImGuiSelectableFlags_::ImGuiSelectableFlags_SpanAllColumns,
			allow_double_click = ImGuiSelectableFlags_::ImGuiSelectableFlags_AllowDoubleClick,
			disabled = ImGuiSelectableFlags_::ImGuiSelectableFlags_Disabled
		};

		//selectables
		//used as elements in comboboxes, etc
		bool selectable(std::string_view label, bool selected, selectable_flag = selectable_flag::none, const vector &size = { 0.f, 0.f });
		bool selectable_easy(std::string_view label, bool &selected, selectable_flag = selectable_flag::none, const vector &size = {0.f, 0.f});

		//listboxes
		//return true if selected item changes
		template<typename Container>
		detail::listbox_with_string<Container> listbox(std::string_view label,
			std::size_t &current_item, const Container&, int height_in_items = -1);

		template<typename Container>
		detail::listbox_no_string<Container> listbox(std::string_view label,
			std::size_t &current_item, const Container&, int height_in_items = -1);

		template<typename Container, typename ToString>
		bool listbox(std::string_view label, std::size_t &current_item,
			const Container&, ToString to_string_func, int height_in_items = -1);

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
		
		//TODO: drags

		//TODO: sliders

		enum class input_text_flags : ImGuiInputTextFlags
		{
			none = ImGuiInputTextFlags_::ImGuiInputTextFlags_None,
			chars_decimal = ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsDecimal,
			chars_hexidecimal = ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsHexadecimal,
			chars_uppercase = ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsUppercase,
			chars_no_blank = ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsNoBlank,
			auto_select_all = ImGuiInputTextFlags_::ImGuiInputTextFlags_AutoSelectAll,
			enter_returns_true = ImGuiInputTextFlags_::ImGuiInputTextFlags_EnterReturnsTrue,
			allow_tab_input = ImGuiInputTextFlags_::ImGuiInputTextFlags_AllowTabInput,
			ctrl_enter_for_newline = ImGuiInputTextFlags_::ImGuiInputTextFlags_CtrlEnterForNewLine,
			no_horizontal_scroll = ImGuiInputTextFlags_::ImGuiInputTextFlags_NoHorizontalScroll,
			always_insert_mode = ImGuiInputTextFlags_::ImGuiInputTextFlags_AlwaysInsertMode,
			readonly = ImGuiInputTextFlags_::ImGuiInputTextFlags_ReadOnly,
			password = ImGuiInputTextFlags_::ImGuiInputTextFlags_Password,
			no_undo_redo = ImGuiInputTextFlags_::ImGuiInputTextFlags_NoUndoRedo,
			chars_scientific = ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsScientific
		};

		//inputs
		//returns true of the value of buffer changes
		template<typename T> // selects the correct function from those bellow
		bool input(std::string_view label, T&, input_text_flags = input_text_flags::none);
		template<std::size_t Size>
		bool input_text(std::string_view label, std::array<char, Size> &buffer, input_text_flags = input_text_flags::none);
		bool input_text(std::string_view label, std::string &buffer, input_text_flags = input_text_flags::none);
		template<std::size_t Size>
		bool input_text_multiline(std::string_view label, std::array<char, Size> &buffer, const vector &size = { 0.f, 0.f }, input_text_flags = input_text_flags::none);
		bool input_text_multiline(std::string_view label, std::string &buffer, const vector &size = {0.f, 0.f}, input_text_flags = input_text_flags::none);
		template<typename T>
		bool input_scalar(std::string_view label, T &v, T step = static_cast<T>(1), T step_fast = static_cast<T>(1), input_text_flags = input_text_flags::none);
		template<typename T, std::size_t Size>
		bool input_scalar_array(std::string_view label, std::array<T, Size> &v, input_text_flags = input_text_flags::none);
		
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
		};

		enum class colour_edit_settings : ImGuiColorEditFlags
		{
			none = ImGuiColorEditFlags_None,
			alpha_bar = ImGuiColorEditFlags_AlphaBar,
			alpha_preview = ImGuiColorEditFlags_AlphaPreview,
			alpha_preview_half = ImGuiColorEditFlags_AlphaPreviewHalf,
			format_hdr = ImGuiColorEditFlags_HDR,
			format_rgb = ImGuiColorEditFlags_RGB,
			format_hsv = ImGuiColorEditFlags_HSV,
			format_hex = ImGuiColorEditFlags_HEX,
			format_uint8 = ImGuiColorEditFlags_Uint8,
			format_float = ImGuiColorEditFlags_Float,
			hue_bar = ImGuiColorEditFlags_PickerHueBar,
			hue_wheel = ImGuiColorEditFlags_PickerHueWheel
		};

		//3/4, 3 = rgb, 4 = rbga
		//edit creates a preview square the summons a picker dialog
		//picker creates a picker widget
		void colour_editor_options(colour_edit_settings);
		bool colour_picker3(std::string_view label, std::array<uint8, 3> &colour, colour_edit_flags = colour_edit_flags::none);
		bool colour_picker4(std::string_view label, std::array<uint8, 4> &colour, colour_edit_flags = colour_edit_flags::none);

		enum class tree_node_flags : ImGuiTreeNodeFlags
		{
			none = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_None,
			selected = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected,
			framed = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Framed,
			allow_item_overlap = ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_AllowItemOverlap,
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
		bool collapsing_header(std::string_view, tree_node_flags = tree_node_flags::none);
		bool collapsing_header(std::string_view, bool &open, tree_node_flags = tree_node_flags::none);

		//TODO: list box

		//TODO: other widgets

		//TODO: plotting

		//TODO: value


		bool main_menubar_begin();
		// Only call if main_menubar_begin() returns true
		void main_menubar_end();
		bool menubar_begin();
		void menubar_end();
		bool menu_begin(std::string_view, bool enabled = true);
		// Only call if menu_begin() returns true 
		void menu_end();
		bool menu_item(std::string_view, bool enabled = true);
		bool menu_toggle_item(std::string_view, bool &selected, bool enabled = true);

		bool main_toolbar_begin();
		void main_toolbar_end();
		bool toolbar_button(std::string_view);
		bool toolbar_button(const resources::animation&);
		void toolbar_separator();

		//tooltips
		void tooltip(std::string_view); //shows a tooltip if the previous item was hovered
		void show_tooltip(std::string_view); //always shows a tooltip

		//TODO: popups
		//popup windows are defined by the popup_begin/popup_end functions
		//they are flagged as open by calling open_popup elsewhere in the program
		//they are otherwise normal windows, though they will close on any input
		//not directed at them
		void open_popup(std::string_view);
		///void popip_begin()
		//only call if popup_begin returned true
		void popup_end();
		void close_current_popup();

		//modal dialogs(block input behind them)
		//same as popups above, but input not directed at them is just ignored
		void open_modal(std::string_view);
		bool modal_begin(std::string_view, window_flags flags = window_flags::none);
		//only call if modal_begin returned true
		void modal_end();
		void close_current_modal();

		//columns
		void columns_begin(std::size_t count = 1u, bool border = true);
		void columns_begin(std::string_view id, std::size_t count = 1u, bool border = true);
		void columns_next();
		void columns_end();

		enum class hovered_flags : ImGuiHoveredFlags
		{
			none = ImGuiHoveredFlags_::ImGuiHoveredFlags_None,
			child_windows = ImGuiHoveredFlags_::ImGuiHoveredFlags_ChildWindows,
			root_window = ImGuiHoveredFlags_::ImGuiHoveredFlags_RootWindow,
			any_window = ImGuiHoveredFlags_::ImGuiHoveredFlags_AnyWindow,
			allow_when_blocked_by_popup = ImGuiHoveredFlags_::ImGuiHoveredFlags_AllowWhenBlockedByPopup,
			allow_when_blocked_by_model = ImGuiHoveredFlags_::ImGuiHoveredFlags_AllowWhenBlockedByActiveItem,
			allow_when_overlapped = ImGuiHoveredFlags_::ImGuiHoveredFlags_AllowWhenOverlapped,
			allow_when_disabled = ImGuiHoveredFlags_::ImGuiHoveredFlags_AllowWhenDisabled,
			rect_only = ImGuiHoveredFlags_::ImGuiHoveredFlags_RectOnly,
			root_and_child_windows = ImGuiHoveredFlags_::ImGuiHoveredFlags_RootAndChildWindows
		};

		//item utils
		bool is_item_hovered(hovered_flags); //returns true if mouse is over object
		bool is_item_focused(); //returns true if item has keyboard or gamepad focus
		vector get_item_rect_max();

		//utils
		vector calculate_text_size(std::string_view, bool include_text_after_double_hash = true, float wrap_width = -1.0f);

		void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

		static constexpr std::string_view version() noexcept;

	private:
		using font = ImFont;

		void _activate_context() noexcept;
		void _active_assert() const noexcept;
		void _toolbar_layout_next();
		static font* _get_font(const resources::font*);
		static font* _create_font(const resources::font*);
		static void _generate_atlas();

		using context = ImGuiContext;

		struct context_destroyer
		{
			void operator()(context *c)
			{
				ImGui::DestroyContext(c);
			}
		};

		using context_ptr = std::unique_ptr<context, context_destroyer>;
		context_ptr _my_context{nullptr};

		struct toolbar_layout_info
		{
			float last_item_x2;
			float main_menubar_y2;
			float width;
		};

		toolbar_layout_info _main_toolbar_info;

		//static font atlas will be shared for all instances of gui
		static std::unique_ptr<ImFontAtlas> _font_atlas;
		static std::unordered_map<const resources::font*, ImFont*> _fonts;
	};

	namespace details
	{
		template<typename Enum>
		constexpr inline Enum enum_or(Enum lhs, Enum rhs) noexcept
		{
			using T = std::underlying_type_t<Enum>;
			return static_cast<Enum>(static_cast<T>(lhs) | static_cast<T>(rhs));
		}
	}

	constexpr inline gui::window_flags operator|(gui::window_flags lhs, gui::window_flags rhs) noexcept
	{
		return details::enum_or(lhs, rhs);
	}

	//TODO: selrctable, combo, etc...
	constexpr inline gui::colour_edit_flags operator|(gui::colour_edit_flags lhs, gui::colour_edit_flags rhs) noexcept
	{
		return details::enum_or(lhs, rhs);
	}

	constexpr inline gui::tree_node_flags operator|(gui::tree_node_flags lhs, gui::tree_node_flags rhs) noexcept
	{
		return details::enum_or(lhs, rhs);
	}

	//NOTE: right_max is absolute, not relative to the current elements x position
	template<typename InputIt, typename MakeButton>
	void gui_make_horizontal_wrap_buttons(gui&, float right_max, InputIt first, InputIt last, MakeButton&&);
}

#include "hades/detail/gui.inl"

#endif //!HADES_GUI_HPP
