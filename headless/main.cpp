#include <tchar.h>

#include <fstream>
#include <iomanip>
#include <unordered_map>

#include "config.hpp"
#include "netvar.hpp"
#include "modules.hpp"

#define CLIENT_MODULE _T("client_panorama.dll")

int main(int argc, char** argv) {
	auto sigs = std::unordered_map<std::string, settings::signature>{};
	auto nets = std::vector<settings::netvar>{};

	if (auto stream = std::ifstream{ "config.json" }; stream.good()) {
		auto config = json::parse(stream);

		for(auto&& netvar_iter : config.at("netvars")) {
			nets.emplace_back(
				netvar_iter.get<settings::netvar>()
			);
		}

		for (auto&& signature_iter : config.at("signatures")) {
			const auto signature = signature_iter.get<settings::signature>();

			sigs[signature.name] = signature;
		}

	} else {
		std::puts("failed to load cfg!");
		return EXIT_FAILURE;
	}

	try {
		auto d = foreign_module{CLIENT_MODULE};
		auto j = json{};

		for(auto && [key, sig] : sigs) {
			j["signatures"][key] = reinterpret_cast<std::uintptr_t>(d.find_pattern(sig));
		}

		// thx hazedumper
		const auto result = d.find_pattern("\xA1\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xA1\x00\x00\x00\x00\xB9", "x????xxxxxxxxxxxx????x");
		// mov eax, g_pClientClassHead	
		const auto g_pClientClassHead = **(ClientClass***)(result + 1u);

#ifdef _DEBUG
		_tprintf(_T("Classes list: 0x%p\n"), result);
		_tprintf(_T("g_pClientClassHead: 0x%p\n"), g_pClientClassHead);
#endif
		auto netvars = netvar_system{ g_pClientClassHead };
	
		for (auto && netvar : nets) {
			j["netvars"][netvar.name] = netvars.get_offset(netvar.table, netvar.prop) + netvar.offset;
		}

		netvars.dump();
		
		std::ofstream o("pretty.json");
		o << std::setw(4) << j << std::endl;

		return EXIT_SUCCESS;
	} catch(const std::runtime_error& error) {

		_tprintf(_T("Failed to initialize dumper: %s\n"), error.what());
		return EXIT_FAILURE;
	}
}
