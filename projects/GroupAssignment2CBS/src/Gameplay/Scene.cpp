#include "Scene.h"

#include <GLFW/glfw3.h>
#include <locale>
#include <codecvt>

#include "Utils/FileHelpers.h"
#include "Utils/GlmBulletConversions.h"

#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Gameplay/MeshResource.h"
#include "Gameplay/Material.h"

#include "Graphics/DebugDraw.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/VertexArrayObject.h"
#include "Application/Application.h"

//OUR INCLUDES
#include <Gameplay/Components/EnemyBehaviour.h>
#include <Gameplay/Components/TargetBehaviour.h>
#include <Gameplay/Components/UIController.h>
#include <Gameplay/Components/EnemySpawnerBehaviour.h>
#include <Gameplay/Components/TargetController.h>
#include <Gameplay/InputEngine.h>
#include <Application/Timing.h>

namespace Gameplay {
	Scene::Scene() :
		_objects(std::vector<GameObject::Sptr>()),
		_deletionQueue(std::vector<std::weak_ptr<GameObject>>()),
		IsPlaying(false),
		IsDestroyed(false),
		MainCamera(nullptr),
		DefaultMaterial(nullptr),
		_isAwake(false),
		_filePath(""),
		_skyboxShader(nullptr),
		_skyboxMesh(nullptr),
		_skyboxTexture(nullptr),
		_skyboxRotation(glm::mat3(1.0f)),
		_ambientLight(glm::vec3(0.1f)),
		_gravity(glm::vec3(0.0f, 0.0f, -9.81f)),
		//OURS
		Enemies(std::vector<GameObject::Sptr>()),
		PlayerLastPosition(glm::vec3(0.0f)),
		EnemySpawnerNames(std::vector<std::string>()),
		IsPaused(false),
		IsPauseUIUp(false),
		IsGameEnd(false),
		IsGameWon(false),
		IsTitleUp(false),
		IsLoseScreenUp(false),
		IsWinScreenUp(false),
		GameStarted(false),
		IsCheatActivated(false),
		IsTutorialFinish(false),
		IsBackGroundPlaying(false),
		GameRound(0),
		EnemiesKilled(0),
		BackgroundLoopCounter(0)
	{
		_lightingUbo = std::make_shared<UniformBuffer<LightingUboStruct>>();
		_lightingUbo->GetData().AmbientCol = glm::vec3(0.1f);
		_lightingUbo->Update();
		_lightingUbo->Bind(LIGHT_UBO_BINDING_SLOT);


		GameObject::Sptr mainCam = CreateGameObject("Main Camera");		
		MainCamera = mainCam->Add<Camera>();

		_InitPhysics();

	}

	Scene::~Scene() {
		MainCamera = nullptr;
		DefaultMaterial = nullptr; 
		_skyboxShader = nullptr;
		_skyboxMesh = nullptr;
		_skyboxTexture = nullptr;
		_objects.clear();
		_components.Clear();
		_CleanupPhysics();
		IsDestroyed = true;
	}

	////// Code Added //////////


	GameObject::Sptr Scene::FindTarget()
	{
		if (Targets.size() != 0) {
			GameObject::Sptr Target = Targets.at(rand() % Targets.size());
			return Target;
		}
		else
		{
			RemoveGameObject(FindObjectByName("Player"));
			IsGameEnd = true;
		}
		return nullptr;
	}

	void Scene::DeleteTarget(const GameObject::Sptr& object)
	{
		std::vector<GameObject::Sptr>::iterator it = std::find(Targets.begin(), Targets.end(), object);
		if (it != Targets.end())
		{
			int index = std::distance(Targets.begin(), it);
			UiControllerObject->Get<UiController>()->UpdateUI();
			Targets.erase(Targets.begin() + index);
			RemoveGameObject(object);
		}
		for (auto Enemy : Enemies) {
			Enemy->Get<EnemyBehaviour>()->NewTarget();
		}
		/// If heart died gameover
		if (object.get()->Name == "Heart") {
			IsGameEnd = true;
		}
	}
	void Scene::DeleteEnemy(const GameObject::Sptr& object)
	{
		std::vector<GameObject::Sptr>::iterator it = std::find(Enemies.begin(), Enemies.end(), object);
		if (it != Enemies.end())
		{
			int index = std::distance(Enemies.begin(), it);
			Enemies.erase(Enemies.begin() + index);
			LOG_INFO("Deleting Object {}", object->Name);
		}
		EnemiesKilled++;
	}

	void Scene::LevellCheck()
	{
		switch (GameRound)
		{
		case 1:
		{
			//Round 2 Spawn
			if (EnemiesKilled == 8) {
				for each (GameObject::Sptr var in Targets)
				{
					var->Get<TargetBehaviour>()->Heal();
				}
				for (auto EnemySpawner : EnemySpawnerObjects) {
					EnemySpawner->Get<EnemySpawnerBehaviour>()->IncreaseEnemySpeed();
					EnemySpawner->Get<EnemySpawnerBehaviour>()->SpawnWave(0, 1, 4);
				}
				GameRound++;
				EnemiesKilled = 0;
			}
			break;
		}
		case 2:
		{
			//Round 3 Spawn
			if (EnemiesKilled == 10) {
				for each (GameObject::Sptr var in Targets)
				{
					var->Get<TargetBehaviour>()->Heal();
				}
				for (auto EnemySpawner : EnemySpawnerObjects) {
					EnemySpawner->Get<EnemySpawnerBehaviour>()->IncreaseEnemySpeed();
					EnemySpawner->Get<EnemySpawnerBehaviour>()->SpawnWave(0, 2, 4);
				}
				GameRound++;
				EnemiesKilled = 0;
			}
			break;
		}
		case 3:
		{
			//Round 4 Spawn
			if (EnemiesKilled == 12) {
				for each (GameObject::Sptr var in Targets)
				{
					var->Get<TargetBehaviour>()->Heal();
				}
				for (auto EnemySpawner : EnemySpawnerObjects) {
					EnemySpawner->Get<EnemySpawnerBehaviour>()->IncreaseEnemySpeed();
					EnemySpawner->Get<EnemySpawnerBehaviour>()->SpawnWave(1, 2, 4);
				}
				GameRound++;
				EnemiesKilled = 0;
			}
			break;
		}
		case 4:
		{
			//Round 5 Spawn
			if (EnemiesKilled == 14) {
				for each (GameObject::Sptr var in Targets)
				{
					var->Get<TargetBehaviour>()->Heal();
				}
				for (auto EnemySpawner : EnemySpawnerObjects) {
					EnemySpawner->Get<EnemySpawnerBehaviour>()->IncreaseEnemySpeed();
					EnemySpawner->Get<EnemySpawnerBehaviour>()->SpawnWave(2, 3, 3);
				}
				GameRound++;
				EnemiesKilled = 0;
			}
			break;
		}
		case 5:
		{
			//Round 6 Spawn
			if (EnemiesKilled == 16) {
				for each (GameObject::Sptr var in Targets)
				{
					var->Get<TargetBehaviour>()->Heal();
				}
				for (auto EnemySpawner : EnemySpawnerObjects) {
					EnemySpawner->Get<EnemySpawnerBehaviour>()->IncreaseEnemySpeed();
					EnemySpawner->Get<EnemySpawnerBehaviour>()->SpawnWave(4, 3, 2);
				}
				GameRound++;
				EnemiesKilled = 0;
			}
			break;
		}
		case 6:
		{
			//Round 7 Spawn
			if (EnemiesKilled == 18) {
				for each (GameObject::Sptr var in Targets)
				{
					var->Get<TargetBehaviour>()->Heal();
				}
				for (auto EnemySpawner : EnemySpawnerObjects) {
					EnemySpawner->Get<EnemySpawnerBehaviour>()->IncreaseEnemySpeed();
					EnemySpawner->Get<EnemySpawnerBehaviour>()->SpawnWave(4, 4, 2);
				}
				GameRound++;
				EnemiesKilled = 0;
			}
			break;
		}
		case 7:
		{
			//Round 8 Spawn
			if (EnemiesKilled == 20) {
				for each (GameObject::Sptr var in Targets)
				{
					var->Get<TargetBehaviour>()->Heal();
				}
				for (auto EnemySpawner : EnemySpawnerObjects) {
					EnemySpawner->Get<EnemySpawnerBehaviour>()->IncreaseEnemySpeed();
					EnemySpawner->Get<EnemySpawnerBehaviour>()->SpawnWave(5, 4, 2);
				}
				GameRound++;
				EnemiesKilled = 0;
			}
			break;
		}
		case 8:
		{
			//Round 9 Spawn
			if (EnemiesKilled == 22) {
				for each (GameObject::Sptr var in Targets)
				{
					var->Get<TargetBehaviour>()->Heal();
				}
				for (auto EnemySpawner : EnemySpawnerObjects) {
					EnemySpawner->Get<EnemySpawnerBehaviour>()->IncreaseEnemySpeed();
					EnemySpawner->Get<EnemySpawnerBehaviour>()->SpawnWave(5, 5, 2);
				}
				GameRound++;
				EnemiesKilled = 0;
			}
			break;
		}
		case 9:
		{
			//Round 10 Spawn
			if (EnemiesKilled == 24) {
				for each (GameObject::Sptr var in Targets)
				{
					var->Get<TargetBehaviour>()->Heal();
				}
				for (auto EnemySpawner : EnemySpawnerObjects) {
					EnemySpawner->Get<EnemySpawnerBehaviour>()->IncreaseEnemySpeed();
					EnemySpawner->Get<EnemySpawnerBehaviour>()->SpawnWave(6, 4, 3);
				}
				GameRound++;
				EnemiesKilled = 0;
			}
			break;
		}
		default:
			if (EnemiesKilled == 26) {
				IsGameWon = true;
				IsGameEnd = true;
			}
			break;
		}

		IsCheatActivated = false;

		if (EnemiesKilled > 26) {
			IsGameWon = true;
			IsGameEnd = true;
		}
	}

	void Scene::GameStart()
	{
		GameRound = 1;
		EnemiesKilled = 0;
		IsCheatActivated = false;

		//Spawn Targets
		TargetSpawnerObject->Get<TargetController>()->Spawntargets();

		//Check if EnemySpawners are here
		if (EnemySpawnerObjects.empty()) {
			for (auto SpawnerName : EnemySpawnerNames) {
				EnemySpawnerObjects.push_back(FindObjectByName(SpawnerName));
			};
		}
		//Spawning first wave of enemies for round 1
		for (auto EnemySpawner : EnemySpawnerObjects) {
			EnemySpawner->Get<EnemySpawnerBehaviour>()->SpawnWave(0, 0, 4);
		}

		//Change UI
		UiControllerObject->Get<UiController>()->SetupGameScreen();
		IsTitleUp = false;

		//Hopefully it works....Yessirski
		audioEngine->playSoundByName("background");
		//Start counter to loopbackground
		IsBackGroundPlaying = true;

		GameStarted = true;
	}

	void Scene::GameWon()
	{
		if (!IsTitleUp && !IsWinScreenUp) {
			UiControllerObject->Get<UiController>()->GameWinScreen();
			IsWinScreenUp = true;
		}
	}

	void Scene::GameOver()
	{
		if (!IsTitleUp && !IsLoseScreenUp) {
			UiControllerObject->Get<UiController>()->GameOverScreen();
			audioEngine->playSoundByName("GameLose");
			IsLoseScreenUp = true;
		}

	}

	void Scene::GamePause(bool IsPaused)
	{
		if (IsPaused && !IsPauseUIUp) {
			UiControllerObject->Get<UiController>()->GamePauseScreen();
			//audioEngine->playSoundByName("menuBackground");
			IsPauseUIUp = true;

		}
		else {
			RemoveGameObject(FindObjectByName("Game Pause"));
			//audioEngine->playSoundByName("background");
			IsPauseUIUp = false;
		}

	}

	//////////////////////////


	void Scene::SetPhysicsDebugDrawMode(BulletDebugMode mode) {
		_bulletDebugDraw->setDebugMode((btIDebugDraw::DebugDrawModes)mode);
	}

	BulletDebugMode Scene::GetPhysicsDebugDrawMode() const {
		return (BulletDebugMode)_bulletDebugDraw->getDebugMode();
	}

	void Scene::SetSkyboxShader(const std::shared_ptr<ShaderProgram>& shader) {
		_skyboxShader = shader;
	}

	std::shared_ptr<ShaderProgram> Scene::GetSkyboxShader() const {
		return _skyboxShader;
	}

	void Scene::SetSkyboxTexture(const std::shared_ptr<TextureCube>& texture) {
		_skyboxTexture = texture;
	}

	std::shared_ptr<TextureCube> Scene::GetSkyboxTexture() const {
		return _skyboxTexture;
	}

	void Scene::SetSkyboxRotation(const glm::mat3& value) {
		_skyboxRotation = value;
	}

	const glm::mat3& Scene::GetSkyboxRotation() const {
		return _skyboxRotation;
	}

	void Scene::SetColorLUT(const Texture3D::Sptr& texture, int slot) {
		if (slot == 0) {
		_colorCorrection = texture;
		}
		else if (slot == 1) {
		_colorWarmCorrection = texture;
		}

	}

	const Texture3D::Sptr& Scene::GetColorLUT(int slot) const {
		if (slot == 0) {
			return _colorCorrection;
		}
		else if (slot == 1) {
			return _colorWarmCorrection;
		}
	}

	GameObject::Sptr Scene::CreateGameObject(const std::string& name)
	{
		GameObject::Sptr result(new GameObject());
		result->Name = name;
		result->_scene = this;
		result->_selfRef = result;
		_objects.push_back(result);
		return result;
	}

	void Scene::RemoveGameObject(const GameObject::Sptr& object) {
		_deletionQueue.push_back(object);
	}

	GameObject::Sptr Scene::FindObjectByName(const std::string name) const {
		auto it = std::find_if(_objects.begin(), _objects.end(), [&](const GameObject::Sptr& obj) {
			return obj->Name == name;
		});
		return it == _objects.end() ? nullptr : *it;
	}

	GameObject::Sptr Scene::FindObjectByGUID(Guid id) const {
		auto it = std::find_if(_objects.begin(), _objects.end(), [&](const GameObject::Sptr& obj) {
			return obj->_guid == id;
		});
		return it == _objects.end() ? nullptr : *it;
	}

	void Scene::SetAmbientLight(const glm::vec3& value) {
		_lightingUbo->GetData().AmbientCol = glm::vec3(0.1f);
		_lightingUbo->Update();

	}

	const glm::vec3& Scene::GetAmbientLight() const { 
		return _lightingUbo->GetData().AmbientCol;
	}

	void Scene::Awake() {
		// Not a huge fan of this, but we need to get window size to notify our camera
		// of the current screen size
		Application& app = Application::Get();
		glm::ivec2 windowSize = app.GetWindowSize();
		if (MainCamera != nullptr) {
			MainCamera->ResizeWindow(windowSize.x, windowSize.y);
		}

		if (_skyboxMesh == nullptr) {
			_skyboxMesh = ResourceManager::CreateAsset<MeshResource>();
			_skyboxMesh->AddParam(MeshBuilderParam::CreateCube(glm::vec3(0.0f), glm::vec3(1.0f)));
			_skyboxMesh->AddParam(MeshBuilderParam::CreateInvert());
			_skyboxMesh->GenerateMesh();
		}

		// Call awake on all gameobjects
		for (auto& obj : _objects) {
			obj->Awake();
		}

		//Code Added
		UiControllerObject = FindObjectByName("UI");
		TargetSpawnerObject = FindObjectByName("Target Spawner");
		/////


		_isAwake = true;
	}

	void Scene::DoPhysics(float dt) {
		_components.Each<Gameplay::Physics::RigidBody>([=](const std::shared_ptr<Gameplay::Physics::RigidBody>& body) {
			body->PhysicsPreStep(dt);
		});
		_components.Each<Gameplay::Physics::TriggerVolume>([=](const std::shared_ptr<Gameplay::Physics::TriggerVolume>& body) {
			body->PhysicsPreStep(dt);
		});

		if (IsPlaying) {

			_physicsWorld->stepSimulation(dt, 1);

			_components.Each<Gameplay::Physics::RigidBody>([=](const std::shared_ptr<Gameplay::Physics::RigidBody>& body) {
				body->PhysicsPostStep(dt);
			});
			_components.Each<Gameplay::Physics::TriggerVolume>([=](const std::shared_ptr<Gameplay::Physics::TriggerVolume>& body) {
				body->PhysicsPostStep(dt);
			});
		}
	}

	void Scene::DrawPhysicsDebug() {
		if (_bulletDebugDraw->getDebugMode() != btIDebugDraw::DBG_NoDebug) {
			_physicsWorld->debugDrawWorld();
			DebugDrawer::Get().FlushAll();
		}
	}

	void Scene::Update(float dt) {
		////OUR CODE
		if (!IsGameEnd)
		{
			_FlushDeleteQueue();
			//Cheats
			if ((InputEngine::GetKeyState(GLFW_KEY_F2) == ButtonState::Pressed) && IsPaused) {
				if (!IsCheatActivated) {
					EnemiesKilled = 100;
					IsCheatActivated = true;
				}
			}
			// Pause
			if (InputEngine::GetKeyState(GLFW_KEY_ESCAPE) == ButtonState::Pressed) {
				if (IsPaused && IsPauseUIUp && IsTutorialFinish)
				{
					IsPaused = false;
					GamePause(IsPaused);
				}
				else
				{
					IsPaused = true;
					GamePause(IsPaused);
				}
			}
			 //Pause help
			if ((InputEngine::GetKeyState(GLFW_KEY_H) == ButtonState::Pressed) && IsPaused) {
				if (IsPaused && IsPauseUIUp && IsTutorialFinish) {
					UiControllerObject->Get<UiController>()->GameTutorial("Pause", 1);
					IsPauseUIUp = false;
					IsTutorialFinish = false;
				}
				else if (IsPaused && !IsPauseUIUp && !IsTutorialFinish)
				{
					UiControllerObject->Get<UiController>()->GameTutorial("Pause", 2);
					IsTutorialFinish = true;
				}
				else {
					RemoveGameObject(FindObjectByName("Tutorial"));
					GamePause(IsPaused);
				}
			}
			//Start
			if (InputEngine::GetKeyState(GLFW_KEY_ENTER) == ButtonState::Pressed) {
				if (!GameStarted && IsTutorialFinish) {
					GameStarted = true;
					GameStart();
				}
				else {
					if (FindObjectByName("Tutorial")&&!IsTutorialFinish) {
						UiControllerObject->Get<UiController>()->GameTutorial("Start", 2);
						IsTutorialFinish = true;
					}
					else if (!IsTutorialFinish)
						UiControllerObject->Get<UiController>()->GameTutorial("Start", 1);
				}
			}
			if (GameStarted) {
				if (IsBackGroundPlaying) {
					BackgroundLoopCounter += Timing::Current().DeltaTime();
					if (BackgroundLoopCounter > 19.0f) {
						audioEngine->playSoundByName("background");
						BackgroundLoopCounter = 0;
					}
				}
				if (!IsPaused) {
					for (int i = 0; i < _objects.size(); i++) {
						_objects[i]->Update(dt);
					}
					if (GameStarted) {
						UiControllerObject->Get<UiController>()->UpdateUI();
						LevellCheck();
					}
				}
			}
			else {
				if (!GameStarted && !IsTitleUp) {
					for (auto Target : Targets) {
						RemoveGameObject(Target);
					}
					for (auto Enemy : Enemies) {
						RemoveGameObject(Enemy);
					}
					UiControllerObject->Get<UiController>()->GameTitleScreen();
					IsTitleUp = true;
				}
			}
		}
		else {
			if (IsGameWon) {
				GameWon();
			}
			else {
				GameOver();
			}
			if (InputEngine::GetKeyState(GLFW_KEY_TAB) == ButtonState::Pressed) {

				Enemies.clear();
				IsGameEnd = false;
				IsGameWon = false;
				GameStarted = false;
				IsTitleUp = false;

				Application& app = Application::Get();
				app.LoadScene("scene.json");
			}
		}




		if (IsPlaying) {

			//No lighitng
			if ((InputEngine::GetKeyState(GLFW_KEY_1) == ButtonState::Pressed)) {
				if (LightCheck == true) {
					SetAmbientLight(glm::vec3(0.0f));

					for (auto light : Lights) { //gets all lgihts mades in default scene 
						light->Get<Light>()->SetIntensity(0.0f);
					}
					LightCheck = false;
				}
				else {
					SetAmbientLight(glm::vec3(0.1f));

					for (auto light : Lights) { //gets all lgihts mades in default scene 
						light->Get<Light>()->SetIntensity(1.0f);
					}
					LightCheck = true;

				}
			}

			//ambient lighitng only
			if ((InputEngine::GetKeyState(GLFW_KEY_2) == ButtonState::Pressed)) {

				if (AmbientLightCheck == true) {
					for (auto light : Lights) {
						light->Get<Light>()->SetIntensity(1.0f);
					}
					AmbientLightCheck = false;
				}
				else {
					for (auto light : Lights) { //gets all lgihts mades in default scene 
						light->Get<Light>()->SetIntensity(0.0f);
					}
					AmbientLightCheck = true;
				}

			}

			//specular lightng only
			if ((InputEngine::GetKeyState(GLFW_KEY_3) == ButtonState::Pressed)) {
				if (SpecularLightCheck == true) {
					for (auto object : _objects) {

						//object->Get<RenderComponent>()->SetMaterial()
					}

					SpecularLightCheck = false;
				}
				else {
					for (auto object : Lights) {
					}

					SpecularLightCheck = true;
				}


			}
			//ambient + specualar
			if ((InputEngine::GetKeyState(GLFW_KEY_4) == ButtonState::Pressed)) {

				if (AmbientLightCheck == true && SpecularLightCheck == true) {

					for (auto light : Lights) {
						light->Get<Light>()->SetIntensity(1.0f);
					}
					AmbientLightCheck = false;
					for (auto object : _objects) {

						//object->Get<RenderComponent>()->SetMaterial()
					}

					SpecularLightCheck = false;
				}
				else {
					for (auto light : Lights) { //gets all lgihts mades in default scene 
						light->Get<Light>()->SetIntensity(0.0f);
					}
					AmbientLightCheck = true;

					for (auto object : _objects) {

						//object->Get<RenderComponent>()->SetMaterial()
					}

					SpecularLightCheck = true;
				}

			}

			//ambient + specular + custom
			if ((InputEngine::GetKeyState(GLFW_KEY_5) == ButtonState::Pressed)) {

				if (AmbientLightCheck == true && SpecularLightCheck == true && customCheck == true) {

					for (auto light : Lights) {
						light->Get<Light>()->SetIntensity(1.0f);
					}
					AmbientLightCheck = false;
					for (auto object : _objects) {

						//object->Get<RenderComponent>()->SetMaterial()
					}

					SpecularLightCheck = false;
				}
				else {
					for (auto light : Lights) { //gets all lgihts mades in default scene 
						light->Get<Light>()->SetIntensity(0.0f);
					}
					AmbientLightCheck = true;

					for (auto object : _objects) {

						//object->Get<RenderComponent>()->SetMaterial()
					}

					SpecularLightCheck = true;
				}

			}
			// toggle diffuse warp/ramp

			// specular warp /ramp

			//toggle colar grade warm
			if ((InputEngine::GetKeyState(GLFW_KEY_8) == ButtonState::Pressed)) {
			//	SetColorLUT(ResourceManager::CreateAsset<Texture3D>("luts/WARM.CUBE"));

			}
			//toggle colr grade cool
			if ((InputEngine::GetKeyState(GLFW_KEY_9) == ButtonState::Pressed)) {
				//SetColorLUT(ResourceManager::CreateAsset<Texture3D>("luts/COOL.CUBE"));

			}
			//toggle color grade custom
			if ((InputEngine::GetKeyState(GLFW_KEY_0) == ButtonState::Pressed)) {
				//SetColorLUT(ResourceManager::CreateAsset<Texture3D>("luts/BW.CUBE"));
			}
		}

		_FlushDeleteQueue();
	}

	void Scene::RenderGUI()
	{
		for (auto& obj : _objects) {
			// Parents handle rendering for children, so ignore parented objects
			if (obj->GetParent() == nullptr) {
				obj->RenderGUI();
			}
		}
	}

	btDynamicsWorld* Scene::GetPhysicsWorld() const {
		return _physicsWorld;
	}

	Scene::Sptr Scene::FromJson(const nlohmann::json& data)
	{

		Scene::Sptr result = std::make_shared<Scene>();
		result->MainCamera = nullptr;
		result->_objects.clear();
		result->DefaultMaterial = ResourceManager::Get<Material>(Guid(data["default_material"]));

		if (data.contains("ambient")) {
			result->SetAmbientLight((data["ambient"]));
		}

		if (data.contains("skybox") && data["skybox"].is_object()) {
			nlohmann::json& blob = data["skybox"].get<nlohmann::json>();
			result->_skyboxMesh = ResourceManager::Get<MeshResource>(Guid(blob["mesh"]));
			result->SetSkyboxShader(ResourceManager::Get<ShaderProgram>(Guid(blob["shader"])));
			result->SetSkyboxTexture(ResourceManager::Get<TextureCube>(Guid(blob["texture"])));
			result->SetSkyboxRotation(glm::mat3_cast((glm::quat)(blob["orientation"])));
		}

		// Make sure the scene has objects, then load them all in!
		LOG_ASSERT(data["objects"].is_array(), "Objects not present in scene!");
		for (auto& object : data["objects"]) {
			GameObject::Sptr obj = GameObject::FromJson(result.get(), object);
			obj->_scene = result.get();
			obj->_parent.SceneContext = result.get();
			obj->_selfRef = obj;
			result->_objects.push_back(obj);
		}

		// Re-build the parent hierarchy 
		for (const auto& object : result->_objects) {
			if (object->GetParent() != nullptr) {
				object->GetParent()->AddChild(object);
			}
		}

		// Create and load camera config
		result->MainCamera = result->_components.GetComponentByGUID<Camera>(Guid(data["main_camera"]));
	
		return result;
	}

	nlohmann::json Scene::ToJson() const
	{
		nlohmann::json blob;
		// Save the default shader (really need a material class)
		blob["default_material"] = DefaultMaterial ? DefaultMaterial->GetGUID().str() : "null";

		blob["ambient"] = GetAmbientLight();

		blob["skybox"] = nlohmann::json();
		blob["skybox"]["mesh"] = _skyboxMesh ? _skyboxMesh->GetGUID().str() : "null";
		blob["skybox"]["shader"] = _skyboxShader ? _skyboxShader->GetGUID().str() : "null";
		blob["skybox"]["texture"] = _skyboxTexture ? _skyboxTexture->GetGUID().str() : "null";
		blob["skybox"]["orientation"] = (glm::quat)_skyboxRotation;


		// Save renderables
		std::vector<nlohmann::json> objects;
		objects.resize(_objects.size());
		for (int ix = 0; ix < _objects.size(); ix++) {
			objects[ix] = _objects[ix]->ToJson();
		}
		blob["objects"] = objects;

		// Save camera info
		blob["main_camera"] = MainCamera != nullptr ? MainCamera->GetGUID().str() : "null";

		return blob;
	}

	void Scene::Save(const std::string& path) {
		_filePath = path;
		// Save data to file
		FileHelpers::WriteContentsToFile(path, ToJson().dump(1, '\t'));
		LOG_INFO("Saved scene to \"{}\"", path);
	}

	Scene::Sptr Scene::Load(const std::string& path)
	{
		LOG_INFO("Loading scene from \"{}\"", path);
		std::string content = FileHelpers::ReadFile(path);
		nlohmann::json blob = nlohmann::json::parse(content);
		Scene::Sptr result = FromJson(blob);
		result->_filePath = path;
		return result;
	}

	int Scene::NumObjects() const {
		return static_cast<int>(_objects.size());
	}

	GameObject::Sptr Scene::GetObjectByIndex(int index) const {
		return _objects[index];
	}

	void Scene::_InitPhysics() {
		_collisionConfig = new btDefaultCollisionConfiguration();
		_collisionDispatcher = new btCollisionDispatcher(_collisionConfig);
		_broadphaseInterface = new btDbvtBroadphase();
		_ghostCallback = new btGhostPairCallback();
		_broadphaseInterface->getOverlappingPairCache()->setInternalGhostPairCallback(_ghostCallback);
		_constraintSolver = new btSequentialImpulseConstraintSolver();
		_physicsWorld = new btDiscreteDynamicsWorld(
			_collisionDispatcher,
			_broadphaseInterface,
			_constraintSolver,
			_collisionConfig
		);
		_physicsWorld->setGravity(ToBt(_gravity));
		// TODO bullet debug drawing
		_bulletDebugDraw = new BulletDebugDraw();
		_physicsWorld->setDebugDrawer(_bulletDebugDraw);
		_bulletDebugDraw->setDebugMode(btIDebugDraw::DBG_NoDebug);
	}

	void Scene::_CleanupPhysics() {
		delete _physicsWorld;
		delete _constraintSolver;
		delete _broadphaseInterface;
		delete _ghostCallback;
		delete _collisionDispatcher;
		delete _collisionConfig;
	}


	void Scene::_FlushDeleteQueue() {
		for (auto& weakPtr : _deletionQueue) {
			if (weakPtr.expired()) continue;
			auto& it = std::find(_objects.begin(), _objects.end(), weakPtr.lock());
			if (it != _objects.end()) {
				_objects.erase(it);
			}
		}
		_deletionQueue.clear();
	}

	void Scene::DrawAllGameObjectGUIs()
	{
		for (auto& object : _objects) {
			object->DrawImGui();
		}

		static char buffer[256];
		ImGui::InputText("", buffer, 256);
		ImGui::SameLine();
		if (ImGui::Button("Add Object")) {
			CreateGameObject(buffer);
			memset(buffer, 0, 256);
		}
	}

	void Scene::DrawSkybox()
	{
		if (_skyboxShader != nullptr &&
			_skyboxMesh != nullptr &&
			_skyboxMesh->Mesh != nullptr &&
			_skyboxTexture != nullptr &&
			MainCamera != nullptr) {
			
			glDepthMask(false);
			glDisable(GL_CULL_FACE);
			glDepthFunc(GL_LEQUAL); 

			_skyboxShader->Bind();
			_skyboxShader->SetUniformMatrix("u_ClippedView", MainCamera->GetProjection());
			_skyboxShader->SetUniformMatrix("u_EnvironmentRotation", _skyboxRotation * glm::inverse(glm::mat3(MainCamera->GetView())));
			_skyboxTexture->Bind(0);
			_skyboxMesh->Mesh->Draw();

			glDepthFunc(GL_LESS);
			glEnable(GL_CULL_FACE);
			glDepthMask(true);

		}
	}

}