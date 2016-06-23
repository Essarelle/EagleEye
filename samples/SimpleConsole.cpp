
#include <EagleLib/rcc/shared_ptr.hpp>
#include <EagleLib/nodes/NodeManager.h>
#include "EagleLib/nodes/Node.h"
#include "EagleLib/Plugins.h"
#include <EagleLib/DataStreamManager.h>
#include <EagleLib/Logging.h>
#include <EagleLib/rcc/ObjectManager.h>
#include <EagleLib/frame_grabber_base.h>
#include <EagleLib/DataStreamManager.h>

#include <signal.h>
#include <signals/logging.hpp>
#include <parameters/Persistence/TextSerializer.hpp>
#include <parameters/IVariableManager.h>
#include <boost/program_options.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/common.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/filesystem.hpp>
#include <boost/version.hpp>
#include <boost/tokenizer.hpp>

#include <cuda.h>
#include <cuda_runtime_api.h>
#include <cuda_runtime.h>

void PrintNodeTree(EagleLib::Nodes::Node* node, int depth)
{
    for(int i = 0; i < depth; ++i)
    {
        std::cout << "=";
    }
    std::cout << node->getFullTreeName() << std::endl;
    for(int i = 0; i < node->children.size(); ++i)
    {
        PrintNodeTree(node->children[i].get(), depth + 1);
    }
}
static volatile bool quit;
void sig_handler(int s)
{
    //std::cout << "Caught signal " << s << std::endl;
	LOG(error) << "Caught signal " << s;
    quit = true;
    if(s == 2)
        exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
	signal(SIGINT, sig_handler);
	signal(SIGILL, sig_handler);
	signal(SIGTERM, sig_handler);
    signal(SIGSEGV, sig_handler);
    
    boost::program_options::options_description desc("Allowed options");
	
    //boost::log::add_file_log(boost::log::keywords::file_name = "SimpleConsole%N.log", boost::log::keywords::rotation_size = 10 * 1024 * 1024);
    EagleLib::SetupLogging();
    
    Signals::thread_registry::get_instance()->register_thread(Signals::GUI);
    desc.add_options()
        ("file", boost::program_options::value<std::string>(), "Optional - File to load for processing")
        ("config", boost::program_options::value<std::string>(), "Optional - File containing node structure")
        ("plugins", boost::program_options::value<boost::filesystem::path>(), "Path to additional plugins to load")
		("log", boost::program_options::value<std::string>()->default_value("info"), "Logging verbosity. trace, debug, info, warning, error, fatal")
        ("mode", boost::program_options::value<std::string>()->default_value("interactive"), "Processing mode, options are interactive or batch")
		("script", boost::program_options::value<std::string>(), "Text file with scripting commands")
        ;

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    
    
    {
        boost::posix_time::ptime initialization_start = boost::posix_time::microsec_clock::universal_time();
        LOG(info) << "Initializing GPU...";
        cv::cuda::GpuMat(10,10, CV_32F);
        boost::posix_time::ptime initialization_end= boost::posix_time::microsec_clock::universal_time();
        if(boost::posix_time::time_duration(initialization_end - initialization_start).total_seconds() > 1)
        {
            cudaDeviceProp props;
            cudaGetDeviceProperties(&props, cv::cuda::getDevice());
            LOG(warning) << "Initialization took " << boost::posix_time::time_duration(initialization_end - initialization_start).total_milliseconds() << " ms.  CUDA code likely not generated for this architecture (" << props.major << "." << props.minor << ")";

        }
    }


    if((!vm.count("config") || !vm.count("file")) && vm["mode"].as<std::string>() == "batch")
    {
        LOG(info) << "Batch mode selected but \"file\" and \"config\" options not set";
        std::cout << desc;
        return 1;
    }
	if (vm.count("log"))
	{
		std::string verbosity = vm["log"].as<std::string>();
		if (verbosity == "trace")
			boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::trace);
		if (verbosity == "debug")
			boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
		if (verbosity == "info")
			boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);
		if (verbosity == "warning")
			boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::warning);
		if (verbosity == "error")
			boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::error);
		if (verbosity == "fatal")
			boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::fatal);
	}else
    {
    	boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);
    }
	boost::filesystem::path currentDir = boost::filesystem::current_path();
#ifdef _MSC_VER
#ifdef _DEBUG
    currentDir = boost::filesystem::path(currentDir.string() + "/../Debug");
#else
    currentDir = boost::filesystem::path(currentDir.string() + "/../RelWithDebInfo");
#endif
#else
    currentDir = boost::filesystem::path(currentDir.string() + "/Plugins");
#endif
    boost::filesystem::directory_iterator end_itr;
	if(boost::filesystem::is_directory(currentDir))
	{
	
	
		for(boost::filesystem::directory_iterator itr(currentDir); itr != end_itr; ++itr)
		{
			if(boost::filesystem::is_regular_file(itr->path()))
			{
#ifdef _MSC_VER
				if(itr->path().extension() == ".dll")
#else
				if(itr->path().extension() == ".so")
#endif
				{
					std::string file = itr->path().string();
					EagleLib::loadPlugin(file);
				}
			}
		}
	}
    boost::thread gui_thread([]
    {
        Signals::thread_registry::get_instance()->register_thread(Signals::GUI);
        while(!boost::this_thread::interruption_requested())
        {
            Signals::thread_specific_queue::run();
            cv::waitKey(1);
        }
    });
	Signals::signal_manager manager;
    if(vm.count("plugins"))
    {
        currentDir = boost::filesystem::path(vm["plugins"].as<boost::filesystem::path>());
        for (boost::filesystem::directory_iterator itr(currentDir); itr != end_itr; ++itr)
        {
            if (boost::filesystem::is_regular_file(itr->path()))
            {
#ifdef _MSC_VER
                if (itr->path().extension() == ".dll")
#else
                if (itr->path().extension() == ".so")
#endif
                {
                    std::string file = itr->path().string();
                    EagleLib::loadPlugin(file);
                }
            }
        }
    }

    if(vm["mode"].as<std::string>() == "batch")
    {
        quit = false;
        std::string document = vm["file"].as<std::string>();
        LOG(info) << "Loading file: " << document;
        std::string configFile = vm["config"].as<std::string>();
        LOG(info) << "Loading config file " << configFile;
    
        auto stream = EagleLib::IDataStream::create(document);
    
        auto nodes = EagleLib::NodeManager::getInstance().loadNodes(configFile);
        stream->AddNodes(nodes);

        LOG(info) << "Loaded " << nodes.size() << " top level nodes";
        for(int i = 0; i < nodes.size(); ++i)
        {
            PrintNodeTree(nodes[i].get(), 1);
        }
        stream->process();
    }else
    {
        std::vector<rcc::shared_ptr<EagleLib::IDataStream>> _dataStreams;
        rcc::weak_ptr<EagleLib::IDataStream> current_stream;
        rcc::weak_ptr<EagleLib::Nodes::Node> current_node;
		Parameters::Parameter* current_param = nullptr;

        auto print_options = []()->void
        {
            std::cout << 
                "- Options: \n"
				" - list_devices     -- List available connected devices for streaming access\n"
				" - load_file        -- Create a frame grabber for a document\n"
				"    document        -- Name of document to load\n"
                " - add              -- Add a node to the current selected object\n"
				"    node            -- name of node, names are listed with list\n"
                " - list             -- List constructable objects\n"
				"    nodes           -- List nodes registered\n"
				" - plugins          -- list all plugins\n"
                " - print            -- Prints the current streams, nodes in current stream, \n"
				"    streams         -- prints all streams\n"
				"    nodes           -- prints all nodes of current stream\n"
				"    parameters      -- prints all parameters of current node or stream\n"
				"    current         -- prints what is currently selected (default)\n"  
				"    signals         -- prints current signal map\n"
				"    inputs          -- prints possible inputs\n"
				" - set              -- Set a parameters value\n"
				"    name value      -- name value pair to be applied to parameter\n"
				" - select           -- Select object\n"
				"    name            -- select node by name\n"
				"    index           -- select stream by index\n"
				"    parameter       -- if currently a node is selected, select a parameter"
				" - delete           -- delete selected item\n"
				" - emit             -- Send a signal\n"
				"    name parameters -- signal name and parameters\n"
                " - save             -- Save node configuration\n"
                " - load             -- Load node configuration\n"
                " - help             -- Print this help\n"
                " - quit             -- Close program and cleanup\n"
                " - log              -- change logging level\n"
                " - link             -- add link directory\n"
			    " - recompile        \n"
                "   check            -- checks if any files need to be recompiled\n"
                "   swap             -- swaps any objects that were recompiled\n";
        };
		std::vector<std::shared_ptr<Signals::connection>> connections;
        //std::map<std::string, std::function<void(std::string)>> function_map;
		connections.push_back(manager.connect<void(std::string)>("list_devices",[](std::string null)->void
		{
			auto constructors = EagleLib::ObjectManager::Instance().GetConstructorsForInterface(IID_FrameGrabber);
			for(auto constructor : constructors)
			{
				auto info = constructor->GetObjectInfo();
				if(info)
				{
					auto fg_info = dynamic_cast<EagleLib::FrameGrabberInfo*>(info);
					if(fg_info)
					{
						auto devices = fg_info->ListLoadableDocuments();
						if(devices.size())
						{
							std::stringstream ss;
							ss << fg_info->GetObjectName() << " can load:\n";
							for(auto& device : devices)
							{
								ss << "    " << device;
							}
							LOG(info) << ss.str();
						}
					}
				}
			}
		}));
		connections.push_back(manager.connect<void(std::string)>("load_file", [&_dataStreams](std::string doc)->void
        {
            if(doc.size() == 0)
            {
                auto stream = EagleLib::IDataStream::create("");
                _dataStreams.push_back(stream);
            }
            if(EagleLib::IDataStream::CanLoadDocument(doc))
            {
                LOG(debug) << "Found a frame grabber which can load " << doc;
                auto stream = EagleLib::IDataStream::create(doc);
                if(stream)
                {
					stream->StartThread();
                    _dataStreams.push_back(stream);
                }else
                {
                    LOG(warning) << "Unable to load document";
                }
            }else
            {
                LOG(warning) << "Unable to find a frame grabber which can load " << doc;
            }
        }));
		connections.push_back(manager.connect<void(std::string)>("quit", [](std::string)->void
        {
            quit = true;
        }));
		
		auto func = [&_dataStreams, &current_stream, &current_node, &current_param](std::string what)->void
		{
			if(what == "streams")
			{
				for(auto& itr : _dataStreams)
				{
					std::cout << " - " << itr->GetPerTypeId() << " - " << itr->GetFrameGrabber()->GetSourceFilename() << "\n";
				}
			}
			if(what == "nodes")
			{
				if(current_stream)
				{
					auto nodes = current_stream->GetNodes();
					for(auto& node : nodes)
					{
						PrintNodeTree(node.get(), 0);
					}
				}
				else if(current_node)
				{
					PrintNodeTree(current_node.get(), 0);
				}
			}
			if(what == "parameters")
			{
				std::vector<Parameters::Parameter*> parameters;
				if(current_node)
				{
					parameters = current_node->getParameters();
				}
				if(current_stream)
				{
					if(auto fg = current_stream->GetFrameGrabber())
						parameters = fg->getParameters();
				}
				for(auto& itr : parameters)
				{
					std::stringstream ss;
					try
					{
						Parameters::Persistence::Text::Serialize(&ss, itr);   
						std::cout << " - " << itr->GetTreeName() << ": " << ss.str() << "\n";
					}catch(...)
					{
						std::cout << " - " << itr->GetTreeName() << "\n";
					}
				}
			}
			if(what == "current" || what.empty())
			{
				if(current_stream)
				{
					if(auto fg = current_stream->GetFrameGrabber())
						std::cout << " - Current stream: " << fg->GetSourceFilename() << "\n";
					else
						std::cout << " - Current stream: " << current_stream->GetPerTypeId() << "\n";
					return;
				}
				if(current_node)
				{
					std::cout << " - Current node: " << current_node->getFullTreeName() << "\n";
					return;
				}
				std::cout << "Nothing currently selected\n";
			}
			if(what == "signals")
			{
				if (current_node)
				{
					current_node->GetDataStream()->GetSignalManager()->print_signal_map();
				}
				if (current_stream)
				{
					current_stream->GetSignalManager()->print_signal_map();
				}
			}
			if(what == "inputs")
			{
				if(current_param && current_node)
				{
					auto potential_inputs = current_node->GetVariableManager()->GetOutputParameters(current_param->GetTypeInfo());
					std::stringstream ss;
					if(potential_inputs.size())
					{
						ss << "Potential inputs: \n";
						for(auto& input : potential_inputs)
						{
							ss << " - " << input->GetName() << "\n";
						}
						LOG(info) << ss.str();
						return;
					}
					LOG(info) << "Unable to find any matching inputs for variable with name: " << current_param->GetName() << " with type: " << current_param->GetTypeInfo().name();
				}
				if(current_node)
				{
					auto params = current_node->getParameters();
					std::stringstream ss;
					for(auto param : params)
					{
						if(param->type & Parameters::Parameter::Input)
						{
							ss << " -- " << param->GetName() << " [ " << param->GetTypeInfo().name() << " ]\n";
							auto potential_inputs = current_node->GetVariableManager()->GetOutputParameters(param->GetTypeInfo());
							for(auto& input : potential_inputs)
							{
								ss << " - " << input->GetTreeName();
							}
						}
					}
					LOG(info) << ss.str();
				}
			}
			if(what == "projects")
			{
				auto project_count = EagleLib::ObjectManager::Instance().getProjectCount();
				std::stringstream ss;
				for(int i = 0; i < project_count; ++i)
				{
					ss << i << " - " << EagleLib::ObjectManager::Instance().getProjectName(i) << "\n";
				}
				LOG(info) << ss.str();
			}
			if(what == "plugins")
			{
				auto plugins = EagleLib::ListLoadedPlugins();
				std::stringstream ss;
				for(auto& plugin : plugins)
				{
					ss << plugin << "\n";
				}
				LOG(info) << ss.str();
			}
		};
		connections.push_back(manager.connect<void(std::string)>("print", func));
		connections.push_back(manager.connect<void(std::string)>("ls", func));
		connections.push_back(manager.connect<void(std::string)>("select", [&_dataStreams,&current_stream, &current_node, &current_param](std::string what)
        {
            int idx = -1;
            std::string name;
            
            try
            {
                idx =  boost::lexical_cast<int>(what);
            }catch(...)
            {
                idx = -1;
            }
            if(idx == -1)
            {
                name = what;
            }
            if(idx != -1)
            {
                LOG(info) << "Selecting stream " << idx;
                for(auto& itr : _dataStreams)
                {
                    if(itr != nullptr)
                    {
                        if(itr->GetPerTypeId() == idx)
                        {
                            current_stream = itr.get();
                            current_node.reset();
                            current_param = nullptr;
                            return;
                        }
                    }
                }
            }
            if(current_stream)
            {
                // look for a node with this name, relative then look absolute
                auto nodes = current_stream->GetNodes();
                for(auto& node : nodes)
                {
                    if(node->getTreeName() == what)
                    {
                        current_stream.reset();
                        current_node = node.get();
                        current_param = nullptr;
                        return;
                    }
                }
                // parse name and try to find absolute path to node
            }
			if(current_node)
			{
				auto child = current_node->getChild(what);
				if(child)
				{
					current_node = child.get();
					current_stream.reset();
					current_param = nullptr;
				}else
				{
					auto params = current_node->getParameters();
					for(auto& param : params)
					{
						if(param->GetName().find(what) != std::string::npos)
						{
							current_param = param;
							current_stream.reset();
						}
					}
				}
			}
        }));
		connections.push_back(manager.connect<void(std::string)>("delete", [&_dataStreams,&current_stream, &current_node](std::string what)
		{
			if(current_stream)
			{
				auto itr = std::find(_dataStreams.begin(), _dataStreams.end(), current_stream.get());
				if(itr != _dataStreams.end())
				{
					_dataStreams.erase(itr);
					current_stream.reset();
					LOG(info) << "Sucessfully deleted stream";
					return;
				}
			}else if(current_node)
			{
				if(auto parent = current_node->getParent())
				{
					parent->removeChild(current_node.get());
					current_node.reset();
					LOG(info) << "Sucessfully removed node from parent node";
					return;
				}else if(auto stream = current_node->GetDataStream())
				{
					stream->RemoveNode(current_node.get());
					current_node.reset();
					LOG(info) << "Sucessfully removed node from datastream";
					return;
				}
			}
			LOG(info) << "Unable to delete item";
		}));
		connections.push_back(manager.connect<void(std::string)>("help", [&print_options](std::string)->void{print_options();}));
		connections.push_back(manager.connect<void(std::string)>("list", [](std::string filter)->void
        {
            auto nodes = EagleLib::NodeManager::getInstance().getConstructableNodes();
            for(auto& node : nodes)
            {
                if(filter.size())
                {
                    if(node.find(filter) != std::string::npos)
                    {
                        std::cout << " - " << node << "\n";
                    }
                }else
                {
                    std::cout << " - " << node << "\n";
                }
                
            }
        }));
		connections.push_back(manager.connect<void(std::string)>("plugins", [](std::string null)->void
		{
			auto plugins = EagleLib::ListLoadedPlugins();
			std::stringstream ss;
			ss << "Loaded / failed plugins:\n";
			for(auto& plugin: plugins)
			{
				ss << "  " << plugin << "\n";
			}
			LOG(info) << ss.str();
		}));
		connections.push_back(manager.connect<void(std::string)>("add", [&current_node, &current_stream](std::string name)->void
        {
			if(current_stream)
			{
				current_stream->AddNode(name);
				return;
			}
			if(current_node)
			{
				EagleLib::NodeManager::getInstance().addNode(name, current_node.get());
			}
        }));
		connections.push_back(manager.connect<void(std::string)>("set", [&current_node, &current_stream, &current_param](std::string value)->void
		{
			if(current_param && current_node && current_param->type & Parameters::Parameter::Input)
			{
				auto variable_manager = current_node->GetVariableManager();
				auto output = variable_manager->GetOutputParameter(value);
				if(output)
				{
					variable_manager->LinkParameters(output, current_param);
					return;
				}
			}
			if(current_param)
			{
                auto pos = value.find(current_param->GetName());
                if(pos != std::string::npos)
                {
                    value = value.substr(current_param->GetName().size());
                }
				if(Parameters::Persistence::Text::DeSerialize(&value, current_param))
                    return;
			}
			if (current_node)
			{
				auto params = current_node->getParameters();
				for(auto& param : params)
				{
					auto pos = value.find(param->GetName());
					if(pos != std::string::npos && value.size() > param->GetName().size() + 1 && value[param->GetName().size()] == ' ')
					{
						LOG(info) << "Setting value for parameter " << param->GetName() << " to " << value.substr(pos + param->GetName().size() + 1);
						std::stringstream ss;
						ss << value.substr(pos + param->GetName().size() + 1);
						Parameters::Persistence::Text::DeSerialize(&ss, param);
						return;
					}
				}
				LOG(info) << "Unable to find parameter by name for set string: " << value;
			}else if(current_stream)
			{
				auto params = current_stream->GetFrameGrabber()->getParameters();
				for(auto& param : params)
				{
					auto pos = value.find(param->GetName());
					if(pos != std::string::npos)
					{
						auto len = param->GetName().size() + 1;
						LOG(info) << "Setting value for parameter " << param->GetName() << " to " << value.substr(len);
						std::stringstream ss;
						ss << value.substr(len);
						Parameters::Persistence::Text::DeSerialize(&ss, param);
						return;
					}
				}
				LOG(info) << "Unable to find parameter by name for set string: " << value;
			}
		}));
		//connections.push_back(manager.connect<void(std::string)>("ls", [&function_map](std::string str)->void {function_map["print"](str); }));
		
		connections.push_back(manager.connect<void(std::string)>("emit", [&current_node, &current_stream](std::string name)
		{
			EagleLib::SignalManager* mgr = nullptr;
			if (current_node)
			{
				mgr = current_node->GetDataStream()->GetSignalManager();
			}
			if (current_stream)
			{
				mgr = current_stream->GetSignalManager();
			}
			std::vector<Signals::signal_base*> signals;
			if (mgr)
			{
				signals = mgr->get_signals(name);
			}
			auto table = PerModuleInterface::GetInstance()->GetSystemTable();
			if (table)
			{
				auto global_signal_manager = table->GetSingleton<EagleLib::SignalManager>();
				if(global_signal_manager)
				{
					auto global_signals = global_signal_manager->get_signals(name);
					signals.insert(signals.end(), global_signals.begin(), global_signals.end());
				}
			}
			if (signals.size() == 0)
			{
				LOG(info) << "No signals found with name: " << name;
				return;
			}
			int idx = 0;
			if (signals.size() > 1)
			{
				for (auto signal : signals)
				{
					std::cout << idx << " - " << signal->get_signal_type().name();
					++idx;
				}
				std::cin >> idx;
			}
			LOG(info) << "Attempting to send signal with name \"" << name << "\" and signature: " << signals[idx]->get_signal_type().name();
            auto proxy = Signals::serialization::text::factory::instance()->get_proxy(signals[idx]);
            if(proxy)
            {
                proxy->send(signals[idx], "");
				delete proxy;
            }
		}));
		connections.push_back(manager.connect<void(std::string)>("log", [](std::string level)
        {
        if (level == "trace")
			boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::trace);
		if (level == "debug")
			boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
		if (level == "info")
			boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);
		if (level == "warning")
			boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::warning);
		if (level == "error")
			boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::error);
		if (level == "fatal")
			boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::fatal);
        }));
		connections.push_back(manager.connect<void(std::string)>("link", [](std::string directory)
        {
            int idx = 0;
            if(auto pos = directory.find(',') != std::string::npos)
            {
                idx = boost::lexical_cast<int>(directory.substr(0, pos));
                directory = directory.substr(pos + 1);
            }
            EagleLib::ObjectManager::Instance().addLinkDir(directory, idx);
        }));
        connections.push_back(manager.connect<void(std::string)>("wait", [](std::string ms) {boost::this_thread::sleep_for(boost::chrono::milliseconds(boost::lexical_cast<int>(ms)));}));
        
        bool swap_required = false;
		connections.push_back(manager.connect<void(std::string)>("recompile", [&current_param, &current_node, &current_stream, &swap_required, &_dataStreams](std::string action)
		{
            if(action == "check")
            {
                EagleLib::ObjectManager::Instance().CheckRecompile();
                boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
                if(EagleLib::ObjectManager::Instance().CheckIsCompiling())
                {
                    LOG(info) << "Recompiling...";
                }
            }
            if(action == "swap")
            {
                for(auto& stream : _dataStreams)
				{
					stream->StopThread();
				}
                if(EagleLib::ObjectManager::Instance().PerformSwap())
                {
                    LOG(info) << "Recompile complete";
                }
                for(auto& stream : _dataStreams)
				{
					stream->StartThread();
				}
                current_param = nullptr;
                current_stream.reset();
                current_node.reset();
            }
            if(action == "abort")
            {
                EagleLib::ObjectManager::Instance().abort_compilation();
            }
		}));
		if (vm.count("file"))
		{
			(*manager.get_signal<void(std::string)>("load_file"))(vm["file"].as<std::string>());
		}
		
		print_options();
        EagleLib::ObjectManager::Instance().CheckRecompile();
		std::vector<std::string> command_list;
		if(vm.count("script"))
		{
			std::ifstream ifs(vm["script"].as<std::string>());
			if(ifs.is_open())
			{
				std::string line;
				while(std::getline(ifs, line))
				{
					command_list.push_back(line);
				}
				if(command_list.size())
					std::reverse(command_list.begin(), command_list.end());
			}
		}
		while(!quit)
        {
            std::string command_line;
			if(command_list.size())
			{
				command_line = command_list.back();
				command_list.pop_back();
			}else
			{
				std::getline(std::cin, command_line);
			}
            
	        std::stringstream ss;
            ss << command_line;
            std::string command;
            std::getline(ss, command, ' ');
			auto signals = manager.get_signals(command);
            if(signals.size() == 1)
            {
                std::string rest;
				std::getline(ss, rest);
                LOG(debug) << "Executing command (" << command << ") with arguments: " << rest;
                try
                {
					LOG(debug) << "Attempting to send signal with name \"" << command << "\" and signature: " << signals[0]->get_signal_type().name();
					auto proxy = Signals::serialization::text::factory::instance()->get_proxy(signals[0]);
					if(proxy)
					{
						proxy->send(signals[0], rest);
						delete proxy;
					}
                }catch(...)
                {
                    LOG(warning) << "Executing command (" << command << ") with arguments: " << rest << " failed miserably";
                }
                
            }else
            {
                LOG(warning) << "Invalid command: " << command_line;
                print_options();
            }
            for(int i = 0; i < 20; ++i)
                Signals::thread_specific_queue::run_once();
        }
        LOG(info) << "Shutting down";
    }
    gui_thread.interrupt();
    gui_thread.join();
    
    return 0;
}
