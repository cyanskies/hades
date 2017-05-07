#include "Hades/Debug.hpp"

#include <algorithm>

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/View.hpp"

#include "Hades/Types.hpp"

namespace hades
{
	namespace debug
	{
		Overlay::Overlay(bool fullscreen) : _fullscreen(fullscreen), _invalid(false)
		{}

		Overlay::~Overlay() {}

		sf::Vector2f Overlay::getSize() const
		{
			_invalid = true;
			return sf::Vector2f();
		}

		void Overlay::setFullscreenSize(sf::Vector2f)
		{
			_invalid = true;
		}

		bool Overlay::fullscreen() const
		{
			return _fullscreen;
		}

		bool Overlay::valid() const
		{
			return _invalid;
		}

		Overlay* OverlayManager::createOverlay(std::unique_ptr<Overlay> overlay)
		{
			if (overlay->fullscreen())
				overlay->setFullscreenSize(_windowSize);

			_overlays.push_back(std::move(overlay));
			return &*_overlays.back();
		}

		Overlay* OverlayManager::destroyOverlay(Overlay *ptr)
		{
			auto loc = std::find_if(_overlays.begin(), _overlays.end(), [ptr](std::unique_ptr<Overlay> &e) {
				return ptr == &*e;
			});

			if (loc != _overlays.end())
			{
				_overlays.erase(loc);
				return nullptr;
			}

			return ptr;
		}

		void OverlayManager::draw(sf::RenderTarget& target, sf::RenderStates states) const
		{
			types::uint32 nextDrawY = 0;

			for (auto &overlay : _overlays)
			{
				if (!overlay->valid())
					continue;

				if (!overlay->fullscreen())
				{
					//arrange the next view
					auto size = overlay->getSize();
					if (size == sf::Vector2f())
						continue;

					sf::View v{ {0.f, static_cast<float>(nextDrawY), size.x, size.y} };
					target.setView(v);
					nextDrawY += static_cast<types::uint32>(size.y);
				}
				overlay->draw(target, states);
			}
		}

		void OverlayManager::setWindowSize(sf::Vector2u size)
		{
			_windowSize = static_cast<sf::Vector2f>(size);

			for (auto &overlay : _overlays)
			{
				if (overlay->fullscreen())
					overlay->setFullscreenSize(_windowSize);
			}
		}

		OverlayManager *overlay_manager = nullptr;

		Overlay *CreateOverlay(std::unique_ptr<Overlay> overlay) 
		{
			if (overlay_manager)
				return overlay_manager->createOverlay(std::move(overlay));

			return nullptr;
		}

		Overlay* DestroyOverlay(Overlay* overlay)
		{
			if (overlay_manager)
				return overlay_manager->destroyOverlay(overlay);

			return overlay;
		}
	}//namespace debug
}//namespace hades
