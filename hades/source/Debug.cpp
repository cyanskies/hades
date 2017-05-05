#include "Hades/Debug.hpp"

#include <algorithm>

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/View.hpp"

#include "Hades/Types.hpp"

namespace hades
{
	namespace debug
	{
		Overlay::Overlay(bool fullscreen) : _fullscreen(fullscreen)
		{}


		Overlay::~Overlay() {}

		sf::Vector2f Overlay::getSize() const
		{
			return sf::Vector2f();
		}

		void Overlay::setFullscreenSize(sf::Vector2f)
		{
			//TODO: this being called should block the overlay from being rendered
		}

		bool Overlay::fullscreen() const
		{
			return _fullscreen;
		}

		Overlay* OverlayManager::createOverlay(std::unique_ptr<Overlay> overlay)
		{
			if (overlay->fullscreen())
				overlay->setFullscreenSize(_windowSize);

			_overlays.push_back(std::move(overlay));
			return &*_overlays.back();
		}

		void OverlayManager::destroyOverlay(Overlay *&ptr)
		{
			auto loc = std::find_if(_overlays.begin(), _overlays.end(), [ptr](std::unique_ptr<Overlay> &e) {
				return ptr == &*e;
			});

			if (loc != _overlays.end())
			{
				ptr = nullptr;
				_overlays.erase(loc);
			}
		}

		void OverlayManager::draw(sf::RenderTarget& target, sf::RenderStates states) const
		{
			types::uint32 nextDrawY = 0;

			for (auto &overlay : _overlays)
			{
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

		void DestroyOverlay(Overlay*& overlay)
		{
			if (overlay_manager)
				return overlay_manager->destroyOverlay(overlay);
		}

	}//namespace debug
}//namespace hades
