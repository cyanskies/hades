#include "Hades/Debug.hpp"

#include <algorithm>
#include <cassert>

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/View.hpp"

#include "Hades/Types.hpp"

namespace hades
{
	namespace debug
	{
		Overlay::Overlay(bool fullscreen) : _fullscreen(fullscreen)
		{}

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
			return !_invalid;
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
			const auto loc = std::find_if(_overlays.begin(), _overlays.end(), [ptr](std::unique_ptr<Overlay> &e) {
				return ptr == &*e;
			});

			if (loc != _overlays.end())
			{
				_overlays.erase(loc);
				return nullptr;
			}

			return ptr;
		}

		void OverlayManager::update()
		{
			for (auto& o : _overlays)
				o->update();
		}

		void OverlayManager::draw(time_duration dt, sf::RenderTarget& target, sf::RenderStates states)
		{
			types::uint32 nextDrawY = 0;

			const auto display = static_cast<sf::Vector2f>(target.getSize());

			for (auto &overlay : _overlays)
			{
				if (!overlay->valid())
					continue;

				if (!overlay->fullscreen())
				{
					//arrange the next view
					const auto size = overlay->getSize();

					sf::View v{ {0.f, 0.f, size.x, size.y} };
					v.setViewport({
						0.f, nextDrawY / display.y,
						size.x / display.x,
						size.y / display.y
						}
					);

					target.setView(v);
					nextDrawY += static_cast<types::uint32>(size.y);
				}
				overlay->draw(dt, target, states);
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
