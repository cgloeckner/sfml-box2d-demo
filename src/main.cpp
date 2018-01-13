#include <iostream>
#include <memory>
#include <iostream>
#include <ctime>
#include <vector>
#include <cmath>

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>

struct GameObject {
	GameObject()
		: face_dir{}
		, body{nullptr} {
	}
	
	sf::Vector2f face_dir;
	b2Body* body;
};

float const tile_size{20.f};

// physics --------------------------------------------------------------------

b2Body* createBody(b2World& world, sf::Vector2f const & pos, bool dynamic, b2Shape* shape) {
	b2BodyDef bodydef;
	bodydef.position = b2Vec2(pos.x, pos.y);
	bodydef.type = dynamic ? b2_dynamicBody : b2_staticBody;
	
	auto body = world.CreateBody(&bodydef);
	
	b2FixtureDef fixdef;
	fixdef.shape = shape;
	body->CreateFixture(&fixdef);

	return body;
}

b2Body* createCirc(b2World& world, sf::Vector2f const & pos, bool dynamic, float radius) {
	b2CircleShape shape;
	shape.m_radius = radius;
	
	return createBody(world, pos, dynamic, &shape);
}


b2Body* createRect(b2World& world, sf::Vector2f const & pos, bool dynamic, sf::Vector2f const & size) {
	b2PolygonShape shape;
	shape.SetAsBox(size.x/2.f, size.y/2.f);

	return createBody(world, pos, dynamic, &shape);
}

float getAngle(b2Vec2 a, b2Vec2 b) {
	auto cross = b2Cross(a, b);
	auto dot = b2Dot(a, b);
	return std::atan2(cross, dot);
}

float getAngle(b2Vec2 face_dir) {
	return getAngle(face_dir, {0.f, -1.f});
}

// ------

class CollisionHandler: public b2ContactListener {
	private:
		void stopBody(b2Body* body);
		
	public:
		void BeginContact(b2Contact* contact);
		void EndContact(b2Contact* contact);
		void PreSolve(b2Contact* contact, b2Manifold const * oldManifold);
		void PostSolve(b2Contact* contact, b2ContactImpulse const * impulse);
};

void CollisionHandler::stopBody(b2Body* body) {
	body->SetLinearVelocity(b2Vec2(0.f, 0.f));
}

void CollisionHandler::BeginContact(b2Contact* contact) {
}

void CollisionHandler::EndContact(b2Contact* contact) {
}

void CollisionHandler::PreSolve(b2Contact* contact, b2Manifold const * oldManifold) {
}

void CollisionHandler::PostSolve(b2Contact* contact, b2ContactImpulse const * impulse) {
	// avoid impulses applied to anyone
	// note: but you still can move the collision opponent (which seems okay)
	stopBody(contact->GetFixtureA()->GetBody());
	stopBody(contact->GetFixtureB()->GetBody());
}

// rendering ------------------------------------------------------------------

void renderCirc(sf::RenderTarget& target, sf::Vector2f const & pos) {
	sf::CircleShape circ{10.f}; // hardcoded for the sake of simplicity
	circ.setOrigin({circ.getRadius(), circ.getRadius()});
	circ.setPosition(pos);
	circ.setOutlineColor(sf::Color::White);
	circ.setFillColor(sf::Color::Transparent);
	circ.setOutlineThickness(1.f);
	target.draw(circ);
}

void renderRect(sf::RenderTarget& target, sf::Vector2f const & pos) {
	sf::RectangleShape rect{{20.f, 20.f}}; // hardcoded for the sake of simplicity
	rect.setOrigin(rect.getSize() / 2.f);
	rect.setPosition(pos);
	rect.setOutlineColor(sf::Color::White);
	rect.setFillColor(sf::Color::Transparent);
	rect.setOutlineThickness(1.f);
	target.draw(rect);
}

// main -----------------------------------------------------------------------

void addWall(sf::VertexArray& vertices, std::vector<b2EdgeShape>& edges, sf::Vector2f const & u, sf::Vector2f const & v) {
	vertices.append({u, sf::Color::White});
	vertices.append({v, sf::Color::White});
	edges.emplace_back();
	edges.back().Set({u.x, u.y}, {v.x, v.y});
}

void populate(GameObject& object, b2World& world, sf::Vector2f const & pos) {
	object.face_dir = {0, 1};
	object.body = createCirc(world, pos, true, 10.f);
}

void populateBox(GameObject& object, b2World& world, sf::Vector2f const & pos) {
	object.body = createRect(world, pos, false, {20.f, 20.f});
}

int main() {
	std::srand(std::time(nullptr));

	// setup world with custom collision handling
	b2Vec2 gravity{0.f, 0.f};
	b2World world{gravity};
	CollisionHandler handler;
	world.SetContactListener(&handler);
	
	std::vector<GameObject> objects;
	objects.resize(3);
	
	// create some objects
	populate(objects[0], world, {400.f, 300.f});
	populate(objects[1], world, {200.f, 400.f});
	populate(objects[2], world, {700.f, 350.f});
	
	// create wall tiles
	sf::VertexArray wall_vertices{sf::Lines};
	std::vector<b2EdgeShape> wall_edges;
	
	addWall(wall_vertices, wall_edges, { 50.f,  50.f}, {750.f,  50.f});
	
	addWall(wall_vertices, wall_edges, { 50.f,  50.f}, { 50.f, 250.f});
	addWall(wall_vertices, wall_edges, { 50.f, 250.f}, {100.f, 250.f});
	addWall(wall_vertices, wall_edges, {100.f, 250.f}, {100.f, 350.f});
	addWall(wall_vertices, wall_edges, {100.f, 350.f}, { 50.f, 350.f});
	addWall(wall_vertices, wall_edges, { 50.f, 350.f}, { 50.f, 550.f});
	
	addWall(wall_vertices, wall_edges, { 50.f, 550.f}, {750.f, 550.f});
	addWall(wall_vertices, wall_edges, {750.f,  50.f}, {750.f, 550.f});
	auto wall = createBody(world, {0.f, 0.f}, false, &wall_edges[0]);
	for (auto i = 1u; i < wall_edges.size(); ++i) {
		wall->CreateFixture(&wall_edges[i], 0.f);
	}
	
	// create random blocks
	for (int i = 0; i < 25; ++i) {
		float x = std::rand() % 600 + 100;
		float y = std::rand() % 400 + 100;
		objects.emplace_back();
		populateBox(objects.back(), world, {x, y});
	}
	
	// create tile border
	// todo: EdgeShape
	
	sf::RenderWindow window{sf::VideoMode{800u, 600u}, "Box2D + SFML Demo"};
	
	bool alternative_movement{false};
	
	while (window.isOpen()) {
		// input stuff
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				window.close();
			}
			if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
				alternative_movement = !alternative_movement;
				std::cout << "toggled controls\n";
			}
		}
		
		// let second and third object track the player
		b2Vec2 dir;
		for (int i = 1u; i <= 2; ++i) {
			dir = objects[0].body->GetPosition() - objects[i].body->GetPosition();
			dir.Normalize();
			objects[i].face_dir = {dir.x, dir.y};
			dir *= 33.f;
			objects[i].body->SetLinearVelocity(dir);
		}
		
		// adjust face_dir towards mouse
		dir.SetZero();
		auto mpos = sf::Mouse::getPosition(window);
		auto opos = objects[0].body->GetPosition();
		dir.x = mpos.x - opos.x;
		dir.y = mpos.y - opos.y;
		dir.Normalize();
		objects[0].face_dir = {dir.x, dir.y};
		
		// apply direction of player according to arrow keys
		dir.SetZero();
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) { dir.y = -1.f; }
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) { dir.y =  1.f; }
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) { dir.x = -1.f; }
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) { dir.x =  1.f; }
		dir.Normalize();
		
		// note: movement relative to facing direction [alternative control setting]
		// toggle with space bar
		if (alternative_movement && (dir.x != 0.f || dir.y != 0.f)) {
			// determine face_dir's rotation angle
			auto angle = getAngle({objects[0].face_dir.x, objects[0].face_dir.y});
			b2Rot r;
			r.Set(-angle); // note: -x due to rotation sense of direction
			dir = b2Mul(r, dir);
		}
		
		// determine whether player is moving backwards
		auto angle = getAngle({objects[0].face_dir.x, objects[0].face_dir.y}, dir) * 180 / 3.14;
		if (abs(angle) > 135.f) {
			// backwards!
			dir *= 0.5f;
		} else if (abs(angle) > 45.f) {
			// sidewards!
			dir *= 0.66f;
		}
		
		// move it!
		dir *= 100.f;
		objects[0].body->SetLinearVelocity(dir);
		
		// simulate world
		world.Step(1/60.f, 8, 3);
		
		// render scene
		window.clear(sf::Color::Black);
		window.draw(wall_vertices);
		for (auto const & obj: objects) {
			auto pos = obj.body->GetPosition();
			// note: I just use one Fixture per GameObject's Body, hence the List contains only the first
			// in production, the rendering systems already knows the exact rendering shape
			if (obj.body->GetFixtureList()->GetType() == b2Shape::e_circle) {
				renderCirc(window, {pos.x, pos.y});
			} else {
				renderRect(window, {pos.x, pos.y});
			}
			
			// draw face_dir
			if (obj.face_dir.x != 0 || obj.face_dir.y != 0) {
				sf::VertexArray vertices{sf::Lines, 2};
				vertices[0].position = {pos.x, pos.y};
				vertices[0].color = sf::Color::Red;
				vertices[1].position = vertices[0].position + sf::Vector2f{obj.face_dir} * 15.f;
				vertices[1].color = sf::Color::Yellow;
				window.draw(vertices);
			}
		}
		window.display();
	}
}

