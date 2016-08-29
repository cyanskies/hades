#include "Hades/ScriptManager.hpp"

//standard library
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

//angelscript
#include "angelscript.h"
#include "scriptarray/scriptarray.h"
#include "scriptstdstring/scriptstdstring.h"
#include "scriptbuilder/scriptbuilder.h"
#include "scripthandle/scripthandle.h"
#include "scriptmath/scriptmath.h"

//SFML
#include "SFML/System.hpp"

//hades
#include "Hades/Console.hpp"
#include "Hades/ResourceManager.hpp"

namespace hades
{
	namespace
	{
		struct includeUserParam
		{
			ResourceManager *r;
			Console *c;
		};


		//scriptbuilder include callback
		int IncludeCallback(const char *include, const char *from, CScriptBuilder *builder, void *userParam)
		{
			assert(include && from && builder && userParam && "invalid parameters passed to IncludeCallback");

			includeUserParam *p = static_cast<includeUserParam*>(userParam);

			assert(p->r && p->c && "userParam does not contain required data");
			std::string includeStr(include);
			//std::string fromStr(from);
			std::shared_ptr<const TextFile> script = p->r->getResource<TextFile>(includeStr);
			if(!script)
			{
				std::cout << "could not find include file: " << include;
				p->c->echo("Could not find or parse script include file: " + std::string(include), Console::ERROR);
				return -1;
			}

			return builder->AddSectionFromMemory(script->GetName().c_str(), script->GetData().c_str());
		}
	}

	ScriptManager::ScriptManager() : _scriptEngine(asCreateScriptEngine(ANGELSCRIPT_VERSION))
	{}

	ScriptManager::~ScriptManager()
	{
		//destroyers don't have to be thread safe(contexts)

		for( unsigned int n = 0; n < contexts.size(); ++n )
			contexts[n]->Release();

		if( _scriptEngine )
			_scriptEngine->Release();
	}

	void ScriptManager::MessageCallback(const asSMessageInfo &msg)
	{
		Console::Console_String_Verbosity type = Console::ERROR;
		if( msg.type == asMSGTYPE_WARNING )
			type = Console::WARNING;
		else if( msg.type == asMSGTYPE_INFORMATION )
			type = Console::NORMAL;

		//TODO: improve display of errored section, place message at line number
		std::stringstream ss;
		ss << msg.section << " (" << msg.row << ", " << msg.col << ") : " << type << " : " << msg.message << std::endl;

		if(console)
			console->echo(ss.str(), type);
		else
			std::cout << ss.str();

		//temp debug code since we cannot switch back to console easily:
		//TODO: get rid of this
		std::cout << ss.str();
		//if( msg.type == asMSGTYPE_ERROR )
			//hasCompileErrors = true;

		return;
	}

	void ScriptManager::init()
	{
		int ret;
		ret = _scriptEngine->SetMessageCallback(asMETHOD(ScriptManager, MessageCallback), this, asCALL_THISCALL); assert( ret >= 0 );

		//register script addons
		RegisterStdString(_scriptEngine);
		RegisterScriptHandle(_scriptEngine);
		RegisterScriptMath(_scriptEngine);
		RegisterScriptArray(_scriptEngine, false);

		auto scriptEngine = _scriptEngine;

		////functions for creating global values
		_globalRegisterFunctions.RegisterGlobalFunction = [scriptEngine](const char* c, const asSFuncPtr& f, asDWORD d){return scriptEngine->RegisterGlobalFunction(c, f, d);};
		_globalRegisterFunctions.RegisterGlobalProperty = [scriptEngine](const char* d, void *v){return scriptEngine->RegisterGlobalProperty(d, v);};
		_globalRegisterFunctions.RegisterFuncdef = [scriptEngine](const char* d){return scriptEngine->RegisterFuncdef(d);};
		_globalRegisterFunctions.RegisterEnumType = [scriptEngine](const char* e){return scriptEngine->RegisterEnum(e);};
		_globalRegisterFunctions.RegisterEnumValue = [scriptEngine](const char* t, const char* n, int v){return scriptEngine->RegisterEnumValue(t, n, v);};

		////store functions for registering C++ class API's
		_classRegisterFunctions.RegisterObjectType = [scriptEngine](const char* c, int i, asDWORD d){return scriptEngine->RegisterObjectType(c, i, d);};
		_classRegisterFunctions.RegisterObjectProperty = [scriptEngine](const char* o, const char* d, int i){ return scriptEngine->RegisterObjectProperty(o, d, i);};
		_classRegisterFunctions.RegisterObjectMethod = [scriptEngine](const char* o, const char* d, asSFuncPtr function, asDWORD convention)
			{return scriptEngine->RegisterObjectMethod(o, d, function, convention, nullptr);};
		_classRegisterFunctions.RegisterObjectBehaviour = [scriptEngine](const char* o, asEBehaviours b, const char* d, asSFuncPtr function, asDWORD convention)
			{return scriptEngine->RegisterObjectBehaviour(o, b, d, function, convention, nullptr);};
	}

	void ScriptManager::registerScriptBaseType(const std::string &scriptname, const std::string &type_name, const ScriptClassAPIFunction registerFunctions)
	{
		int r;

		asIScriptModule *mod = _scriptEngine->GetModule(scriptname.c_str(), asGM_ONLY_IF_EXISTS);
		if( mod )
		{
			// We've already attempted loading the script before
			console->echo("RegisterBaseType: script already loaded("
				+ scriptname + "), for type(" + type_name + ")", Console::WARNING);
			return;
		}

		// Compile the script into the module
		CScriptBuilder builder;
		r = builder.StartNewModule(_scriptEngine, scriptname.c_str()); assert(r >= 0);

		std::shared_ptr<const TextFile> file = resource->getResource<TextFile>(scriptname);
		if(file == nullptr)
		{
			console->echo("Failed to read script file: " + scriptname, Console::ERROR);
			return;
		}

		includeUserParam p;
		p.r = &*resource;
		p.c = &*console;
		//specify how to handle include directives(our goal is to load them through the resource manager)
		builder.SetIncludeCallback(IncludeCallback, &p);

		// Let the builder load the script, and do the necessary pre-processing (include files, etc)
		//we'll load the script from memor
		//r = builder.AddSectionFromFile((script).c_str());
		r = builder.AddSectionFromMemory(file->GetName().c_str(), file->GetData().c_str()); //to use asset manager
		assert(r >= 0); //TODO: need to replace these with proper fail conditions

		r = builder.BuildModule();
		assert(r >= 0); //TODO: and this one too needs a proper fail condition

		mod = _scriptEngine->GetModule(scriptname.c_str(), asGM_ONLY_IF_EXISTS);
		int tc = mod->GetObjectTypeCount();
		for( int n = 0; n < tc; n++ )
		{
			asIObjectType *type = mod->GetObjectTypeByIndex(n);
			if(strcmp(type->GetName(), type_name.c_str()) == 0)
			{
				ScriptClassInfo info = {type, registerFunctions};
				_baseTypes.insert(std::make_pair(type_name, info));
				break;
			}
		}
	}

	bool ScriptManager::createScriptClassController(const std::string &scriptname, Uses_Scripts *object, const std::string &baseName, ScriptClassAPIFunction registerFunctions)
	{
		if(scriptname.empty())
			return false;

		auto ctrl = getControllerScript(scriptname);
		if(!ctrl)
		{
			console->echo("Failed to compile script", Console::ERROR);
			return false;
		}

		asIScriptModule *mod = _scriptEngine->GetModule(scriptname.c_str(), asGM_ONLY_IF_EXISTS);

		asIObjectType *type = 0;
		ScriptClassInfo baseClass = {nullptr, nullptr};
		ScriptObject objectData;
		int tc = mod->GetObjectTypeCount();
		bool inherits = false;

		if(baseName.empty())
		{
			if(tc > 0) //at least one class defined
			{
				type = mod->GetObjectTypeByIndex(0); // get the frist class, should be only one per scriptname anyway
				objectData.type = type;
			}
		}
		else
		{
			inherits = true;
			auto baseIter = _baseTypes.find(baseName);

			if(baseIter != _baseTypes.end())
				baseClass = baseIter->second;
			else
			{
				console->echo("Base class " + baseName + " not registered.", Console::ERROR);
				return false;
			}

			bool found = false;
			for( int n = 0; n < tc; n++ )
			{
				found = false;
				type = mod->GetObjectTypeByIndex(n);
				//int ic = type->GetInterfaceCount();
				if( type->DerivesFrom(baseClass.objectType) )
				{
					found = true;
					objectData.type = type;
					break;
				}
			}

			if(!found)
			{
				console->echo("Couldn't find the controller class for the type '" + scriptname + "'," +
					scriptname + " must derive from " + baseName, Console::ERROR);
				controllers.pop_back();
				return false;
			}
		}

		// Find the factory function
		std::string s = std::string(type->GetName()) + "@ " + std::string(type->GetName()) + "()";
		ctrl->factoryFunc = type->GetFactoryByDecl(s.c_str());
		if( !ctrl->factoryFunc)
		{
			std::cout << "Couldn't find the appropriate factory function for the type '" << scriptname << "'" << std::endl;
			controllers.pop_back();
			//delete ctrl;
			return false;
		}

		if(registerFunctions)
			registerFunctions(ctrl->functions, std::bind(&asIObjectType::GetMethodByDecl, type, std::placeholders::_1, true));
		else
		{
			if(inherits)
				baseClass.function(ctrl->functions, std::bind(&asIObjectType::GetMethodByDecl, type, std::placeholders::_1, true));
			else
				console->echo("No functions bound for script: " + scriptname, Console::WARNING);
		}

		asIScriptObject *obj = nullptr;
		// Create the object using the factory function
		asIScriptContext *ctx = prepareContextFromPool(ctrl->factoryFunc);

		// Make the call and take care of any errors that may happen
		int r = executeCall(ctx);
		if( r == asEXECUTION_FINISHED )
		{
			// Get the newly created object
			obj = *((asIScriptObject**)ctx->GetAddressOfReturnValue());

			// Since a reference will be kept to this object
			// it is necessary to increase the ref count
			obj->AddRef();
		}

		// Return the context to the pool so it can be reused
		returnContextToPool(ctx);

		objectData.object = obj;

		object->attach(objectData);
		object->attach(ctrl);

		return true;
	}

	void ScriptManager::registerGlobals(GlobalAPIFunction &defineAPI)
	{
		defineAPI(_globalRegisterFunctions);
	}

	void ScriptManager::registerClass(const std::string &className, const ClassAPIFunction &defineAPI)
	{
		auto type = _classes.find(className);
		if(type != _classes.end())
		{
			console->echo("ScriptManager::ResgisterClass: class " + className + " already defined", Console::WARNING);
		}
		else
			_classes.insert(className);

		defineAPI(_classRegisterFunctions);
	}

	asIObjectType * const ScriptManager::getObjectType(const std::string &name) const
	{
		return _scriptEngine->GetObjectTypeByDecl(name.c_str());
	}

	bool ScriptManager::buildGenericObject(std::shared_ptr<ScriptFunctions> controller, std::shared_ptr<ScriptObject> ScriptObject)
	{
		asIScriptObject *obj = nullptr;
		// Create the object using the factory function
		asIScriptContext *ctx = prepareContextFromPool(controller->factoryFunc);

		// Pass the object pointer to the script function. With this call the
		// context will automatically increase the reference count for the object.
		//ctx->SetArgObject(0, gameObj);

		// Make the call and take care of any errors that may happen
		int r = executeCall(ctx);
		if( r == asEXECUTION_FINISHED )
		{
			// Get the newly created object
			obj = *((asIScriptObject**)ctx->GetAddressOfReturnValue());

			// Since a reference will be kept to this object
			// it is necessary to increase the ref count
			obj->AddRef();
		}

		// Return the context to the pool so it can be reused
		returnContextToPool(ctx);

		ScriptObject->object = obj;

		return controller != nullptr;
	}

	std::shared_ptr<ScriptFunctions> ScriptManager::getControllerScript(const std::string &script)
	{

		//TODO: remove duplicated code using CScriptBuilder here and in another method above.
		int r;

		// Find the cached controller
		for( unsigned int n = 0; n < controllers.size(); n++ )
		{
			if( controllers[n]->module == script )
				return controllers[n];
		}

		// No controller, check if the script has already been loaded
		asIScriptModule *mod = _scriptEngine->GetModule(script.c_str(), asGM_ONLY_IF_EXISTS);
		if( mod )
		{
			// We've already attempted loading the script before, but there is no controller
			return 0;
		}

		// Compile the script into the module
		CScriptBuilder builder;
		r = builder.StartNewModule(_scriptEngine, script.c_str());
		if( r < 0 )
			return 0;

		std::shared_ptr<const TextFile> file = resource->getResource<TextFile>(script);
		if(file == nullptr)
		{
			console->echo("Failed to read script file: " + script, Console::ERROR);
			return nullptr;
		}

		includeUserParam p;
		p.r = &*resource;
		p.c = &*console;
		//specify how to handle include directives(our goal is to load them through the resource manager)
		builder.SetIncludeCallback(IncludeCallback, &p);

		// Let the builder load the script, and do the necessary pre-processing (include files, etc)
		//we'll load the script from memor
		//r = builder.AddSectionFromFile((script).c_str());
		r = builder.AddSectionFromMemory(file->GetName().c_str(), file->GetData().c_str()); //top use asset manager
		if( r < 0 )
			return 0;

		r = builder.BuildModule();
		if( r < 0 )
			return 0;

		// Cache the functions and methods that will be used
		//ScriptFunctions *ctrl = new ScriptFunctions;
		std::shared_ptr<ScriptFunctions> ctrl(std::make_shared<ScriptFunctions>());
		ctrl->module = script;
		controllers.push_back(ctrl);

		return ctrl;
	}

	void ScriptManager::runScript(asIScriptFunction* functionid, asIScriptObject* object)
	{
		// Call the method using the shared context
		if(functionid)
		{
			asIScriptContext *ctx = prepareContextFromPool(functionid);

			if(object)
				ctx->SetObject(object);

			executeCall(ctx);
			returnContextToPool(ctx);
		}

	}

	asIScriptContext * const ScriptManager::getContext(asIScriptFunction* functionId)
	{
		if(functionId)
			return prepareContextFromPool(functionId);

		return nullptr;
	}

	void ScriptManager::returnContext(asIScriptContext* ctx)
	{
		returnContextToPool(ctx);
	}

	int ScriptManager::executeCall(asIScriptContext* ctx)
	{
		int r = ctx->Execute();
		if( r != asEXECUTION_FINISHED )
		{
			if( r == asEXECUTION_EXCEPTION )
			{
				std::stringstream ss;
				ss << "Exception: " << ctx->GetExceptionString() << std::endl;
				ss << "Function: " << ctx->GetExceptionFunction()->GetDeclaration() << std::endl;
				ss << "Line: " << ctx->GetExceptionLineNumber() << std::endl;

				std::cout << ss.str();


				//TODO: print this to the console
				// It is possible to print more information about the location of the
				// exception, for example the call stack, values of variables, etc if
				// that is of interest.
			}
		}

		return r;
	}

	asIScriptContext* ScriptManager::prepareContextFromPool(asIScriptFunction* funcId)
	{
		asIScriptContext *ctx;

		{
			std::lock_guard<std::mutex> lock(contextMutex);
			if( contexts.size() )
			{
				ctx = *contexts.rbegin();
				contexts.pop_back();
			}
			else
				ctx = _scriptEngine->CreateContext();
		}

		int r = ctx->Prepare(funcId); assert( r >= 0 );

		return ctx;
	}

	void ScriptManager::returnContextToPool(asIScriptContext* ctx)
	{	
		// Unprepare the context to free any objects that might be held
		// as we don't know when the context will be used again.
		ctx->Unprepare();
		std::lock_guard<std::mutex> lock(contextMutex);
		contexts.push_back(ctx);
	}
}//hades
