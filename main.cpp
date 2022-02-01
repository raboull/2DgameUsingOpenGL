#define _USE_MATH_DEFINES
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include "Geometry.h"
#include "GLDebug.h"
#include "Log.h"
#include "ShaderProgram.h"
#include "Shader.h"
#include "Texture.h"
#include "Window.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_angle.hpp >

//An example struct for Game Objects.
//You are encouraged to customize this as you see fit.
struct GameObject {
	// Struct's constructor deals with the texture.
	// Also sets default position, theta, scale, and transformationMatrix
	GameObject(std::string texturePath, GLenum textureInterpolation, glm::vec3 inPosition) :
		texture(texturePath, textureInterpolation),
		position(inPosition),
		theta(0.0f),
		prevTheta(0.0f),
		prevPosition(0.0f, 0.0f, 0.0f),
		remainingRotation(0.0f),
		rotationStep(0.0f),
		rotatedSoFar(0.0f),
		scale(1.0f),
		transformationMatrix(1.0f), //This constructor sets it as the identity matrix
		rotationInProgress(false),
		shiftInProgress(false),
		directionalMove(0.0f, 0.0f, 0.0f),
		usedForScaleUpdate(false),
		numOfSteps(0),
		directionCode(NULL)		
	{}

	CPU_Geometry cgeom;
	GPU_Geometry ggeom;
	Texture texture;

	void gameObjectHello() {
		std::cout << "Hello from game object!" << std::endl;
	}

	//this updates the theta of the object to face the clicked location
	void angle2click(glm::vec2 clicked_location) {

		glm::vec3 clickPosition = glm::vec3(clicked_location, 0.f);//convert clicked location to a vec3 type

		//shift the click position the same amount it will take to move the ship to the origin
		glm::mat4 translationToOrigin = glm::translate(glm::mat4(1.0f), (position-directionalMove));
		clickPosition = translationToOrigin * glm::vec4(clickPosition, 0.0f);
		prevTheta = theta;//store the theta of the object before it began rotating

		//compute the angle from the clicked location to the ship heading
		float angle = atan2(clickPosition.x - position.x, clickPosition.y - position.y);//x and y parameters swapped to accomodate for ships initial heading, might refactor since this is a universal game object struct not just for the ship
		theta = -angle;//angle of rotation required for the ship to face the clicked location in a RHCS
	}

	void setRemainingRotation() {
		remainingRotation = (float)theta - (float)prevTheta;
		rotationStep = remainingRotation / 1000;
		rotatedSoFar = 0.0f;
	}

	void makeTransformationMatrix() {
		glm::mat4 scalingMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale));
		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), theta, glm::vec3(0.0, 0.0, 1.0));
		glm::vec3 rotatedDirectionalMove = rotationMatrix * glm::vec4(directionalMove, 1.0f);
		glm::mat4 shiftToPosition = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 shiftToOrigin = glm::translate(glm::mat4(1.0f), -1.0f * (position + rotatedDirectionalMove));
		transformationMatrix = shiftToPosition*rotationMatrix*scalingMatrix;
	}

	void pickUp(GameObject& diamond1, GameObject& diamond2, GameObject& diamond3, GameObject& diamond4, int& scoreCounter) {

		//get a vector of the current ship location by applying the current transformation Matrix to the position vector
		glm::vec3 tempPosition = position;

		//compute distance from ship to each diamond
		float dist1 = glm::length(diamond1.position - tempPosition);
		float dist2 = glm::length(diamond2.position - tempPosition);
		float dist3 = glm::length(diamond3.position - tempPosition);
		float dist4 = glm::length(diamond4.position - tempPosition);

		//dissapear the diamond if ship passes close by
		if (dist1 < 0.2f) {
			diamond1.scale = 0.001f;
		}
		if (dist2 < 0.2f) {
			diamond2.scale = 0.001f;
		}
		if (dist3 < 0.2f) {
			diamond3.scale = 0.001f;
		}
		if (dist4 < 0.2f) {
			diamond4.scale = 0.001f;
		}

		//update scale of the ship
		int num_of_items = 0;//count the number of diamons that are considered to be picked up
		if (diamond1.scale == 0.001f && diamond1.usedForScaleUpdate == false) {
			num_of_items = num_of_items + 1;
			diamond1.usedForScaleUpdate = true;
			scoreCounter = scoreCounter + 1;
		}
		if (diamond2.scale == 0.001f && diamond2.usedForScaleUpdate == false) {
			num_of_items = num_of_items + 1;
			diamond2.usedForScaleUpdate = true;
			scoreCounter = scoreCounter + 1;
		}
		if (diamond3.scale == 0.001f && diamond3.usedForScaleUpdate == false) {
			num_of_items = num_of_items + 1;
			diamond3.usedForScaleUpdate = true;
			scoreCounter = scoreCounter + 1;
		}
		if (diamond4.scale == 0.001f && diamond4.usedForScaleUpdate == false) {
			num_of_items = num_of_items + 1;
			diamond4.usedForScaleUpdate = true;
			scoreCounter = scoreCounter + 1;
		}
		scale = scale + 0.03f * num_of_items;
	}

	glm::vec3 position;
	float theta;//Object's rotation
	float prevTheta;//previous rotatation orientation of the object
	glm::vec3 prevPosition;
	float remainingRotation;
	float rotationStep;
	float rotatedSoFar;
	float scale;
	glm::mat4 transformationMatrix;
	bool rotationInProgress;
	bool shiftInProgress;
	glm::vec3 directionalMove;
	bool usedForScaleUpdate;
	int numOfSteps;
	char directionCode;
};

//this struct holds information about mouse controls
struct MouseState {
	glm::vec2 mouse_coord;
	glm::vec2 mouse_down_coord;
	glm::vec2 mouse_up_coord;
	bool mouse_clicked = false;
	bool mouse_release = false;
	bool mouse_held = false;
	bool state_changed = true;
};

//this struct holds information about keyboard controls
struct KeyboardState {
	bool up_pressed = false;
	bool down_pressed = false;
	bool n_pressed = false;
	bool state_changed = true;
};

class MyCallbacks : public CallbackInterface {
public:
	MyCallbacks(ShaderProgram& shader, int screen_width, int screen_height) :
		shader(shader),
		screen_dimensions(screen_width, screen_height)
	{}
	//this function handles keyboard button events
	virtual void keyCallback(int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_R && action == GLFW_PRESS) {//screen refresh with a button callback
			shader.recompile();
		}
		else if (key == GLFW_KEY_UP && action == GLFW_PRESS) {//this button press will ultimately move the ship up along its heading direction
			keyboardState.up_pressed = true;
		}
		else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {//this button press will ultimately move the ship down along its heading direction
			keyboardState.down_pressed = true;
		}
		else if (key == GLFW_KEY_N && action == GLFW_PRESS) {//this button press will ultimately start a new game
			keyboardState.n_pressed = true;
		}
		keyboardState.state_changed = true;
	}

	//this function handles mouse button events
	virtual void mouseButtonCallback(int button, int action, int mods) {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			mouseState.mouse_down_coord = mouseState.mouse_coord;
			mouseState.mouse_clicked = true;
			mouseState.mouse_held = true;
		}
		else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {}
		else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
			mouseState.mouse_up_coord = mouseState.mouse_coord;
			mouseState.mouse_release = true;
			mouseState.mouse_held = false;
		}
		else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {}
		mouseState.state_changed = true;
	}

	//this function handles mouse movement events
	virtual void cursorPosCallback(double xpos, double ypos) {
		mouseState.mouse_coord = glm::vec2(xpos, ypos);
		mouseState.mouse_coord = mouseState.mouse_coord / (screen_dimensions - 1.f);//component wise operations
		mouseState.mouse_coord *= 2;
		mouseState.mouse_coord -= 1.f;
		mouseState.mouse_coord.y = -mouseState.mouse_coord.y;//invert the y-axis coordinates
		mouseState.state_changed = true;
	}

	bool mouseStateChanged() {
		return mouseState.state_changed;
	}

	void mouseStateHandled() {
		mouseState.state_changed = false;
		mouseState.mouse_clicked = false;
	}

	//getter for our mouse state
	const MouseState& getMouseState() {
		return mouseState;
	}

	bool keyboardStateChanged() {
		return keyboardState.state_changed;
	}

	void keyboardStateHandled() {
		keyboardState.state_changed = false;
		keyboardState.up_pressed = false;
		keyboardState.down_pressed = false;
		keyboardState.n_pressed = false;
	}

	//getter for our keyboard state
	const KeyboardState& getKeyboardState() {
		return keyboardState;
	}

private:
	ShaderProgram& shader;
	//used by mouse events
	glm::vec2 screen_dimensions;
	MouseState mouseState;
	//used by keyboard events
	KeyboardState keyboardState;
};

//create geometry of the ship
CPU_Geometry shipGeom(float width, float height) {
	float halfWidth = width / 2.0f;
	float halfHeight = height / 2.0f;
	CPU_Geometry retGeom;

	// For full marks (Part IV), you'll need to use the following vertex coordinates instead.
	// Then, you'd get the correct scale/translation/rotation by passing in uniforms into
	// the vertex shader.
	retGeom.verts.push_back(glm::vec3(-1.f, 1.f, 0.f));	//Top left coordinate
	retGeom.verts.push_back(glm::vec3(-1.f, -1.f, 0.f));//Bottom left coorindate
	retGeom.verts.push_back(glm::vec3(1.f, -1.f, 0.f));	//Bottom right coordinate
	retGeom.verts.push_back(glm::vec3(-1.f, 1.f, 0.f));	//Top left coordinate
	retGeom.verts.push_back(glm::vec3(1.f, -1.f, 0.f));	//Bottom right coordinate
	retGeom.verts.push_back(glm::vec3(1.f, 1.f, 0.f));	//Top right coordinate
	
	// texture coordinates
	retGeom.texCoords.push_back(glm::vec2(0.f, 1.f));
	retGeom.texCoords.push_back(glm::vec2(0.f, 0.f));
	retGeom.texCoords.push_back(glm::vec2(1.f, 0.f));
	retGeom.texCoords.push_back(glm::vec2(0.f, 1.f));
	retGeom.texCoords.push_back(glm::vec2(1.f, 0.f));
	retGeom.texCoords.push_back(glm::vec2(1.f, 1.f));
	return retGeom;
}

//create geometry of diamond
CPU_Geometry diamondGeom(float width, float height, glm::vec3 position) {
	float halfWidth = width / 2.0f;
	float halfHeight = height / 2.0f;
	CPU_Geometry retGeom;

	// For full marks (Part IV), you'll need to use the following vertex coordinates instead.
	// Then, you'd get the correct scale/translation/rotation by passing in uniforms into
	// the vertex shader.
	retGeom.verts.push_back(glm::vec3(-1.f, 1.f, 0.f));
	retGeom.verts.push_back(glm::vec3(-1.f, -1.f, 0.f));
	retGeom.verts.push_back(glm::vec3(1.f, -1.f, 0.f));
	retGeom.verts.push_back(glm::vec3(-1.f, 1.f, 0.f));
	retGeom.verts.push_back(glm::vec3(1.f, -1.f, 0.f));
	retGeom.verts.push_back(glm::vec3(1.f, 1.f, 0.f));
	
	// texture coordinates
	retGeom.texCoords.push_back(glm::vec2(0.f, 1.f));
	retGeom.texCoords.push_back(glm::vec2(0.f, 0.f));
	retGeom.texCoords.push_back(glm::vec2(1.f, 0.f));
	retGeom.texCoords.push_back(glm::vec2(0.f, 1.f));
	retGeom.texCoords.push_back(glm::vec2(1.f, 0.f));
	retGeom.texCoords.push_back(glm::vec2(1.f, 1.f));
	return retGeom;
}

int main() {
	Log::debug("Starting main");

	// WINDOW
	glfwInit();
	int screen_width = 800;
	int screen_height = 800;
	Window window(screen_width, screen_height, "CPSC 453"); // can set callbacks at construction if desired

	GLDebug::enable();

	// SHADERS
	ShaderProgram shader("shaders/test.vert", "shaders/test.frag");

	// CALLBACKS
	auto callback_controller = std::make_shared<MyCallbacks>(shader, screen_width, screen_height);
	window.setCallbacks(callback_controller); // can also update callbacks to new ones

	// GL_NEAREST looks a bit better for low-res pixel art than GL_LINEAR.
	// But for most other cases, you'd want GL_LINEAR interpolation.
	GameObject ship("textures/ship.png", GL_NEAREST, glm::vec3(0.0f, 0.0f, 0.0f));
	ship.position = glm::vec3(-0.f, -0.f, 0.f);
	ship.scale = 0.08;
	ship.theta = glm::radians(0.0f);
	ship.cgeom = shipGeom(0.f, 0.f);//set up CPU Geometry
	ship.ggeom.setVerts(ship.cgeom.verts);//setup GPU Geometry
	ship.ggeom.setTexCoords(ship.cgeom.texCoords);//setup Texture coordinates

	//create parameters for diamond objects
	//GameObject diamond("textures/diamond.png", GL_NEAREST, glm::vec3(-0.8f, 0.8f, 0.0f));//create a GameObject struct to represent an instance of a diamond
	GameObject diamond("textures/diamond.png", GL_NEAREST, glm::vec3(0.f, 0.f, 0.f));//create a GameObject struct to represent an instance of a diamond
	diamond.position = glm::vec3(-0.5f, 0.5f, 0.0f);//update diamond location
	diamond.scale = 0.06;//set the scale of this diamond
	diamond.theta = glm::radians(0.f);
	diamond.cgeom = diamondGeom(0.f, 0.f, diamond.position);//set up CPU Geometry
	diamond.ggeom.setVerts(diamond.cgeom.verts);//setup GPU Geometry
	diamond.ggeom.setTexCoords(diamond.cgeom.texCoords);//setup Texture coordinates

	//create parameters for a second diamond objects
	//GameObject diamond("textures/diamond.png", GL_NEAREST, glm::vec3(-0.8f, 0.8f, 0.0f));//create a GameObject struct to represent an instance of a diamond
	GameObject diamond2("textures/diamond.png", GL_NEAREST, glm::vec3(0.f, 0.f, 0.f));//create a GameObject struct to represent an instance of a diamond
	diamond2.position = glm::vec3(-0.5f, -0.5f, 0.0f);//update diamond location
	diamond2.scale = 0.06;//set the scale of this diamond
	diamond2.theta = glm::radians(0.f);
	diamond2.cgeom = diamondGeom(0.f, 0.f, diamond2.position);//set up CPU Geometry
	diamond2.ggeom.setVerts(diamond2.cgeom.verts);//setup GPU Geometry
	diamond2.ggeom.setTexCoords(diamond2.cgeom.texCoords);//setup Texture coordinates

	//create parameters for a third diamond objects
	GameObject diamond3("textures/diamond.png", GL_NEAREST, glm::vec3(0.f, 0.f, 0.f));//create a GameObject struct to represent an instance of a diamond
	diamond3.position = glm::vec3(0.5f, 0.5f, 0.0f);//update diamond location
	diamond3.scale = 0.06;//set the scale of this diamond
	diamond3.theta = glm::radians(0.f);
	diamond3.cgeom = diamondGeom(0.f, 0.f, diamond3.position);//set up CPU Geometry
	diamond3.ggeom.setVerts(diamond3.cgeom.verts);//setup GPU Geometry
	diamond3.ggeom.setTexCoords(diamond3.cgeom.texCoords);//setup Texture coordinates

	//create parameters for a 4th diamond objects
	GameObject diamond4("textures/diamond.png", GL_NEAREST, glm::vec3(0.f, 0.f, 0.f));//create a GameObject struct to represent an instance of a diamond
	diamond4.position = glm::vec3(0.5f, -0.5f, 0.0f);//update diamond location
	diamond4.scale = 0.06;//set the scale of this diamond
	diamond4.theta = glm::radians(0.f);
	diamond4.cgeom = diamondGeom(0.f, 0.f, diamond4.position);//set up CPU Geometry
	diamond4.ggeom.setVerts(diamond4.cgeom.verts);//setup GPU Geometry
	diamond4.ggeom.setTexCoords(diamond4.cgeom.texCoords);//setup Texture coordinates

	//initialize controller peripheral state objects
	const MouseState& mouseState = callback_controller->getMouseState();
	const KeyboardState& keyboardState = callback_controller->getKeyboardState();

	int score = 0;//initialize our score counter

	// RENDER LOOP
	while (!window.shouldClose()) {
		glfwPollEvents();

		//watch for mouse events
		if (callback_controller->mouseStateChanged()) {
			//update state
			if(mouseState.mouse_clicked){//on click
				//compute angle between ship coordinates and clicked coordinates
				ship.angle2click(mouseState.mouse_coord);
				ship.rotationInProgress = true;//set rotation flag
				ship.setRemainingRotation();//called to initialize a rotation parameter
				ship.makeTransformationMatrix();
			}
			callback_controller->mouseStateHandled();//turn off once the state change is addressed because we don't want to update every frame, just when changes occur
		}
		//watch for keyboard events
		if (callback_controller->keyboardStateChanged()) {
			//update state
			if (keyboardState.up_pressed) {//on press
				//update ship location
				ship.prevPosition = ship.position;
				glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), ship.theta, glm::vec3(0.0, 0.0, 1.0));
				glm::vec3 rotatedDirectionalMove = rotationMatrix * glm::vec4(ship.directionalMove, 1.0f);
				ship.position = ship.position + rotatedDirectionalMove;
				ship.shiftInProgress = true;//set shift flag
				ship.numOfSteps = 0;//reset step counter
				ship.directionCode = 'U';
				ship.pickUp(diamond, diamond2, diamond3, diamond4, score);//pickup game object if they are close by
			}
			else if(keyboardState.down_pressed){//on press
				//update ship location
				ship.prevPosition = ship.position;
				glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), ship.theta, glm::vec3(0.0, 0.0, 1.0));
				glm::vec3 rotatedDirectionalMove = rotationMatrix * glm::vec4(ship.directionalMove, 1.0f);
				ship.position = ship.position - rotatedDirectionalMove;
				ship.shiftInProgress = true;//set shift flag
				ship.numOfSteps = 0;//reset step counter
				ship.directionCode = 'D';
				ship.pickUp(diamond, diamond2, diamond3, diamond4, score);//pickup game object if they are close by
			}
			else if (keyboardState.n_pressed) {//on press
				//reset the ship location parameters
				ship.position = glm::vec3(0.f, 0.f, 0.f);
				ship.scale = 0.08;
				ship.theta = glm::radians(0.0f);
				ship.directionalMove = glm::vec3(0.0f, 0.0f, 0.0f);

				//reset the diamonds location parameters
				diamond.scale = 0.06;//reset scale
				diamond.usedForScaleUpdate = false;
				diamond2.scale = 0.06;
				diamond2.usedForScaleUpdate = false;
				diamond3.scale = 0.06;
				diamond3.usedForScaleUpdate = false;
				diamond4.scale = 0.06;
				diamond4.usedForScaleUpdate = false;

				//reset the score counter
				score = 0;
			}
			callback_controller->keyboardStateHandled();
		}

		//setup draw parameters
		shader.use();
		GLint locationPosition = glGetUniformLocation(shader.getID(), "position");//set up the shader reference and the variable we are pasing something to
		GLint locationTheta = glGetUniformLocation(shader.getID(), "theta");
		GLint locationTransformationMatrix = glGetUniformLocation(shader.getID(), "transformationMatrix");
		glEnable(GL_FRAMEBUFFER_SRGB);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//draw the ship
		ship.ggeom.bind();
		ship.texture.bind();
		ship.makeTransformationMatrix();
		glUniformMatrix4fv(locationTransformationMatrix, 1, false, glm::value_ptr(ship.transformationMatrix));
		//rotate the ship if a mouse was clicked
		if (ship.rotationInProgress == true) {
			ship.remainingRotation = ship.remainingRotation - ship.rotationStep;
			ship.rotatedSoFar = ship.rotatedSoFar + ship.rotationStep;
			ship.theta = ship.prevTheta + ship.rotatedSoFar;
			ship.makeTransformationMatrix();
			if (abs(ship.remainingRotation) < 0.01) {
				ship.rotationInProgress = false;
			}
		}
		//move the ship forward if an up arrow key is pressed
		else if (ship.shiftInProgress == true && ship.directionCode == 'U') {
			glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), ship.theta, glm::vec3(0.0, 0.0, 1.0));
			glm::vec3 rotatedDirectionalMove = rotationMatrix * glm::vec4(glm::vec3(0.0f, 0.0005f, 0.0f), 1.0f);
			ship.position = ship.position + rotatedDirectionalMove;
			ship.makeTransformationMatrix();
			if (ship.numOfSteps < 400)
				ship.numOfSteps++;
			else {
				ship.shiftInProgress = false;
				ship.directionCode = NULL;
			}
			ship.pickUp(diamond, diamond2, diamond3, diamond4, score);
		}
		//move the ship backwards if a down arrow key is pressed
		else if (ship.shiftInProgress == true && ship.directionCode == 'D') {
			glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), ship.theta, glm::vec3(0.0, 0.0, 1.0));
			glm::vec3 rotatedDirectionalMove = rotationMatrix * glm::vec4(glm::vec3(0.0f, -0.0005f, 0.0f), 1.0f);
			ship.position = ship.position + rotatedDirectionalMove;
			ship.makeTransformationMatrix();
			if (ship.numOfSteps < 400)
				ship.numOfSteps++;
			else {
				ship.shiftInProgress = false;
				ship.directionCode = NULL;
			}
			ship.pickUp(diamond, diamond2, diamond3, diamond4, score);
		}

		glDrawArrays(GL_TRIANGLES, 0, 6);
		ship.texture.unbind();

		//draw the first diamond
		diamond.ggeom.bind();
		diamond.texture.bind();
		diamond.makeTransformationMatrix();
		glUniformMatrix4fv(locationTransformationMatrix, 1, false, glm::value_ptr(diamond.transformationMatrix));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		diamond.texture.unbind();
		//draw the second diamond
		diamond2.ggeom.bind();
		diamond2.texture.bind();
		diamond2.makeTransformationMatrix();
		glUniformMatrix4fv(locationTransformationMatrix, 1, false, glm::value_ptr(diamond2.transformationMatrix));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		diamond2.texture.unbind();
		//draw the 3rd diamond
		diamond3.ggeom.bind();
		diamond3.texture.bind();
		diamond3.makeTransformationMatrix();
		glUniformMatrix4fv(locationTransformationMatrix, 1, false, glm::value_ptr(diamond3.transformationMatrix));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		diamond3.texture.unbind();
		//draw the 4th diamond
		diamond4.ggeom.bind();
		diamond4.texture.bind();
		diamond4.makeTransformationMatrix();
		glUniformMatrix4fv(locationTransformationMatrix, 1, false, glm::value_ptr(diamond4.transformationMatrix));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		diamond4.texture.unbind();	

		glDisable(GL_FRAMEBUFFER_SRGB); // disable sRGB for things like imgui

		// Starting the new ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		// Putting the text-containing window in the top-left of the screen.
		ImGui::SetNextWindowPos(ImVec2(5, 5));

		// Setting flags
		ImGuiWindowFlags textWindowFlags =
			ImGuiWindowFlags_NoMove |				// text "window" should not move
			ImGuiWindowFlags_NoResize |				// should not resize
			ImGuiWindowFlags_NoCollapse |			// should not collapse
			ImGuiWindowFlags_NoSavedSettings |		// don't want saved settings mucking things up
			//ImGuiWindowFlags_AlwaysAutoResize |	// window should auto-resize to fit the text
			ImGuiWindowFlags_NoBackground |			// window should be transparent; only the text should be visible
			ImGuiWindowFlags_NoDecoration |			// no decoration; only the text should be visible
			ImGuiWindowFlags_NoTitleBar;			// no title; only the text should be visible

		// Begin a new window with these flags. (bool *)0 is the "default" value for its argument.
		ImGui::Begin("scoreText", (bool *)0, textWindowFlags);

		// Scale up text a little, and set its value
		ImGui::SetWindowFontScale(1.5f);
		if(score <4)
			ImGui::Text("Score: %d", score); // Second parameter gets passed into "%d"
		else {//display a congradutatory message
			ImGui::SetWindowPos(ImVec2(70, 350));//update position of message
			ImGui::SetWindowSize(ImVec2(800, 500));
			ImGui::SetWindowFontScale(5.0f);//update size of font
			ImGui::Text("You won! Press N."); 
		}
		// End the window.
		ImGui::End();

		ImGui::Render();	// Render the ImGui window
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); // Some middleware thing

		window.swapBuffers();
	}
	// ImGui cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}
