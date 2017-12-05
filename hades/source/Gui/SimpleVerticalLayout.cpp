#include "Gui/SimpleVerticalLayout.hpp"

namespace hades
{
	namespace gui
	{
		SimpleVerticalLayout::SimpleVerticalLayout(const tgui::Layout2d& size) : tgui::VerticalLayout(size)
		{
			m_type = "SimpleVerticalLayout";
		}
		SimpleVerticalLayout::Ptr SimpleVerticalLayout::create(const tgui::Layout2d& size)
		{
			return std::make_shared<SimpleVerticalLayout>(size);
		}

		SimpleVerticalLayout::Ptr SimpleVerticalLayout::copy(SimpleVerticalLayout::ConstPtr layout)
		{
			if (layout)
				return std::static_pointer_cast<SimpleVerticalLayout>(layout->clone());
			else
				return nullptr;
		}

		void SimpleVerticalLayout::updateWidgets()
		{
			float currentVertOffset = 0.f;
			float currentLineMaxVertOffset = 0.f;
			for (std::size_t i = 0; i < m_widgets.size(); ++i)
			{
				auto size = m_widgets[i]->getSize();

				m_widgets[i]->setPosition(0.f, currentVertOffset);

				if (m_widgets[i]->getFullSize() != m_widgets[i]->getSize())
				{
					m_widgets[i]->setPosition(m_widgets[i]->getPosition() + m_widgets[i]->getWidgetOffset());
				}

				currentVertOffset += size.y;
			}
		}
	}
}

