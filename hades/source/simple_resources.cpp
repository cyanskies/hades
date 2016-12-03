#include "Hades/simple_resources.hpp"

#include "Hades/DataManager.hpp"
#include "Hades/resource/texture.hpp"

namespace hades
{
	namespace resources
	{
		void parseTexture(data::UniqueId mod, YAML::Node& node, data::data_manager*)
		{
			//default texture yaml
			//textures: {
			//    default: {
			//        width: 0 #0 = autodetect, no size checking will be done
			//        height: 0 
			//        source: #empty source, no file is specified, will always be default error texture
			//        smooth: false
			//        repeating: false
			//    }
			//}

			const texture::size_type d_width = 0, d_height = 0;
			const bool default = false;

			//set the loader to loadTexture
		}

		void loadTexture(resources::resource_base*, data::data_manager*)
		{
		}
	}
}