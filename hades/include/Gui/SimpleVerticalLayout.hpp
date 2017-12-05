#ifndef HADES_GUI_SIMPLE_VERTICAL_LAYOUT_HPP
#define HADES_GUI_SIMPLE_VERTICAL_LAYOUT_HPP

#include <TGUI/Widgets/VerticalLayout.hpp>

namespace hades
{
	namespace gui
	{
		class  SimpleVerticalLayout : public tgui::VerticalLayout
		{
		public:
			typedef std::shared_ptr<SimpleVerticalLayout> Ptr; ///< Shared widget pointer
			typedef std::shared_ptr<const SimpleVerticalLayout> ConstPtr; ///< Shared constant widget pointer

			SimpleVerticalLayout(const tgui::Layout2d& size = { "100%", "100%" });

			static SimpleVerticalLayout::Ptr create(const tgui::Layout2d& size = { "100%", "100%" });

			static SimpleVerticalLayout::Ptr copy(ConstPtr layout);

		protected:

			void updateWidgets() override;

			tgui::Widget::Ptr clone() const override
			{
				return std::make_shared<SimpleVerticalLayout>(*this);
			}
		};
	}
}

#endif // HADES_GUI_SIMPLE_VERTICAL_LAYOUT_HPP
