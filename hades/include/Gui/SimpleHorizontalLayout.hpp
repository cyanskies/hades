#ifndef HADES_GUI_SIMPLE_HORIZONTAL_LAYOUT_HPP
#define HADES_GUI_SIMPLE_HORIZONTAL_LAYOUT_HPP

#include <TGUI/Widgets/HorizontalLayout.hpp>

namespace hades
{
	namespace gui
	{
		class  SimpleHorizontalLayout : public tgui::HorizontalLayout
		{
		public:
			typedef std::shared_ptr<SimpleHorizontalLayout> Ptr; ///< Shared widget pointer
			typedef std::shared_ptr<const SimpleHorizontalLayout> ConstPtr; ///< Shared constant widget pointer

			SimpleHorizontalLayout(const tgui::Layout2d& size = { "100%", "100%" });

			static SimpleHorizontalLayout::Ptr create(const tgui::Layout2d& size = { "100%", "100%" });

			static SimpleHorizontalLayout::Ptr copy(ConstPtr layout);

		protected:

			void updateWidgets() override;

			tgui::Widget::Ptr clone() const override
			{
				return std::make_shared<SimpleHorizontalLayout>(*this);
			}
		};
	}
}

#endif // HADES_GUI_SIMPLE_HORIZONTAL_LAYOUT_HPP
