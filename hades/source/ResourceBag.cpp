#include "Hades/ResourceBag.hpp"

namespace hades
{
	namespace data
	{
		void resource_base::load(data_manager* datamanager)
		{
			if (_resourceLoader)
			{
				_resourceLoader(this, datamanager);
				_generation++;
			}
		}
	}
}