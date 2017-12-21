#include "Gui/SimpleHorizontalLayout.hpp"

namespace hades
{
	namespace gui
	{
		SimpleHorizontalLayout::SimpleHorizontalLayout(const tgui::Layout2d& size) : tgui::HorizontalLayout(size)
		{
			m_type = "SimpleHorizontalLayout";
		}
		SimpleHorizontalLayout::Ptr SimpleHorizontalLayout::create(const tgui::Layout2d& size)
		{
			return std::make_shared<SimpleHorizontalLayout>(size);
		}

		SimpleHorizontalLayout::Ptr SimpleHorizontalLayout::copy(SimpleHorizontalLayout::ConstPtr layout)
		{
			if (layout)
				return std::static_pointer_cast<SimpleHorizontalLayout>(layout->clone());
			else
				return nullptr;
		}

		void SimpleHorizontalLayout::updateWidgets()
		{
			float currentHoriOffset = 0.f;
			for (std::size_t i = 0; i < m_widgets.size(); ++i)
			{
				auto size = m_widgets[i]->getSize();

				m_widgets[i]->setPosition(currentHoriOffset, 0.f);

				if (m_widgets[i]->getFullSize() != m_widgets[i]->getSize())
				{
					m_widgets[i]->setPosition(m_widgets[i]->getPosition() + m_widgets[i]->getWidgetOffset());
				}

				currentHoriOffset += size.x;
			}
		}
	}
}

