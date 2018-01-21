#include <iostream>
#include <memory>
#include <iostream>
#include <ctime>
#include <vector>
#include <cmath>

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>
#include <Thor/Math.hpp>
#include <Thor/Vectors.hpp>

class ArcShape: public sf::CircleShape {
	private:
		float angle_span;
		sf::Vector2f direction;
		
	public:
		ArcShape(float radius=0.f, std::size_t point_count=30u);

		void setAngle(float angle);
		void setDirection(sf::Vector2f const & v);
		
		float getAngle() const;
		sf::Vector2f getDirection() const;
		
		virtual sf::Vector2f getPoint(std::size_t index) const;
};

ArcShape::ArcShape(float radius, std::size_t point_count)
	: sf::CircleShape{radius, point_count}
	, angle_span{}
	, direction{1.f, 0.f} {
	setAngle(360.f);
}

void ArcShape::setAngle(float angle) {
	angle_span = thor::toRadian(angle);
	update();
}

void ArcShape::setDirection(sf::Vector2f const & v) {
	direction = v;
	update();
}

float ArcShape::getAngle() const {
	return angle_span;
}

sf::Vector2f ArcShape::getPoint(std::size_t index) const {
	static const float pi = 3.141592654f;
	auto const radius = getRadius();

	auto angle = index * 2.f * pi / getPointCount();
	float x{0.f}, y{0.f};
	
	if (0.f <= angle && angle <= angle_span) {	
		angle += thor::toRadian(thor::polarAngle(direction));
		angle -= angle_span / 2.f;
		
		x = std::cos(angle) * radius;
		y = std::sin(angle) * radius;
	}
	return {radius + x, radius + y};
}

// ----------------------------------------------------------------

struct GameObject {
	GameObject()
		: face_dir{}
		, body{nullptr}
		, is_projectile{false}
		, is_enemy{false}
		, drop{false}
		, respawn{false} {
	}
	
	sf::Vector2f face_dir;
	b2Body* body;
	bool is_projectile, is_enemy;
	float radius;
	sf::Vector2f size;
	
	ArcShape fov;
	
	bool drop, respawn;
};

// physics --------------------------------------------------------------------

b2Body* createBody(b2World& world, sf::Vector2f const & pos, bool dynamic, b2Shape* shape, bool bullet) {
	b2BodyDef bodydef;
	bodydef.bullet = bullet;
	bodydef.position = b2Vec2(pos.x, pos.y);
	bodydef.type = dynamic ? b2_dynamicBody : b2_staticBody;
	
	auto body = world.CreateBody(&bodydef);
	
	b2FixtureDef fixdef;
	fixdef.shape = shape;
	body->CreateFixture(&fixdef);

	return body;
}

b2Body* createCirc(b2World& world, sf::Vector2f const & pos, bool dynamic, float radius, bool bullet) {
	b2CircleShape shape;
	shape.m_radius = radius;
	
	return createBody(world, pos, dynamic, &shape, bullet);
}

b2Body* createRect(b2World& world, sf::Vector2f const & pos, bool dynamic, sf::Vector2f const & size, bool bullet) {
	b2PolygonShape shape;
	shape.SetAsBox(size.x/2.f, size.y/2.f);

	return createBody(world, pos, dynamic, &shape, bullet);
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
	auto a = contact->GetFixtureA()->GetBody()->GetUserData();
	auto b = contact->GetFixtureB()->GetBody()->GetUserData();
	GameObject* target{nullptr};
	if (a != nullptr) {
		auto obj = reinterpret_cast<GameObject*>(a);
		if (obj->is_projectile) {
			obj->drop = true;
			target = reinterpret_cast<GameObject*>(b);
		}
	}
	if (b != nullptr) {
		auto obj = reinterpret_cast<GameObject*>(b);
		if (obj->is_projectile) {
			obj->drop = true;
			target = reinterpret_cast<GameObject*>(a);
		}
	}
	if (target != nullptr && target->is_enemy) {
		// trgger teleport target (fake respawn)
		target->respawn = true;
		std::cout << "hit!\n";
	}
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

void renderCirc(sf::RenderTarget& target, sf::Vector2f const & pos, float radius) {
	sf::CircleShape circ{radius};
	circ.setOrigin({circ.getRadius(), circ.getRadius()});
	circ.setPosition(pos);
	circ.setOutlineColor(sf::Color::White);
	circ.setFillColor(sf::Color::Transparent);
	circ.setOutlineThickness(1.f);
	target.draw(circ);
}

void renderRect(sf::RenderTarget& target, sf::Vector2f const & pos, sf::Vector2f const & size) {
	sf::RectangleShape rect{size};
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

void populate(std::unique_ptr<GameObject>& object, b2World& world, sf::Vector2f const & pos) {
	object->face_dir = {0, 1};
	object->radius = 10.f;
	object->body = createCirc(world, pos, true, object->radius, false);
	object->body->SetUserData(object.get());
	object->fov.setRadius(20.f);
	object->fov.setFillColor(sf::Color::Transparent);
	object->fov.setOutlineThickness(1.f);
	object->fov.setOutlineColor(sf::Color::Yellow);
	object->fov.setOrigin({20.f, 20.f});
	object->fov.setAngle(120.f);
}

void populateBox(std::unique_ptr<GameObject>& object, b2World& world, sf::Vector2f const & pos) {
	object->size = {20.f, 20.f};
	object->body = createRect(world, pos, false, object->size, false);
	object->body->SetUserData(object.get());
}

void populateProjectile(std::unique_ptr<GameObject>& object, b2World& world, sf::Vector2f const & pos, sf::Vector2f dir) {
	object->radius = 2.f;
	object->body = createCirc(world, pos, true, object->radius, true);
	dir *= 200.f;
	object->body->SetLinearVelocity({dir.x, dir.y});
	object->body->SetUserData(object.get());
	object->is_projectile = true;
}

template <typename T>
bool pop(std::vector<T>& container, typename std::vector<T>::iterator i,
	bool stable) {
	auto end = std::end(container);
	if (i == end) {
		return false;
	}
	// move element to container's back
	if (stable) {
		// swap hand over hand
		auto j = i;
		while (++j != end) {
			std::swap(*i, *j);
			++i;
		}
	} else {
		// swap immediately
		auto last = std::prev(end);
		std::swap(*i, *last);
	}
	container.pop_back();
	return true;
}

int main() {
	std::srand(std::time(nullptr));

	// setup world with custom collision handling
	b2Vec2 gravity{0.f, 0.f};
	b2World world{gravity};
	CollisionHandler handler;
	world.SetContactListener(&handler);
	
	std::vector<std::unique_ptr<GameObject>> objects;
	objects.reserve(2000);
	
	// create player
	objects.emplace_back(std::make_unique<GameObject>());
	populate(objects.back(), world, {400.f, 300.f});
	
	// create enemies
	for (auto i = 0u; i < 10; ++i) {
		objects.emplace_back(std::make_unique<GameObject>());
		float x = std::rand() % 600 + 100;
		float y = std::rand() % 400 + 100;
		populate(objects.back(), world, {x, y});
		objects.back()->is_enemy = true;
	}
	
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
	auto wall = createBody(world, {0.f, 0.f}, false, &wall_edges[0], false);
	for (auto i = 1u; i < wall_edges.size(); ++i) {
		wall->CreateFixture(&wall_edges[i], 0.f);
	}
	
	// create random blocks
	for (int i = 0; i < 25; ++i) {
		float x = std::rand() % 600 + 100;
		float y = std::rand() % 400 + 100;
		objects.emplace_back(std::make_unique<GameObject>());
		populateBox(objects.back(), world, {x, y});
	}
	
	// create tile border
	// todo: EdgeShape
	
	sf::RenderWindow window{sf::VideoMode{800u, 600u}, "Box2D + SFML Demo"};
	window.setFramerateLimit(100u);
	unsigned short frames = 0u;
	unsigned short time = 0u;
	sf::Clock clock;
	
	bool alternative_movement{false};
	std::vector<decltype(objects.begin())> cleanup;
	cleanup.reserve(2000);
	
	sf::VertexArray bullets{sf::Points};
	bullets.resize(1000);
	bullets.clear(); // pseudo-reserve
	
	
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
			if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
				std::cout << "peng\n";
				objects.emplace_back(std::make_unique<GameObject>());
				auto pos = objects[0]->body->GetPosition();
				// adjust position to avoid collision with player
				auto fdir = b2Vec2{objects[0]->face_dir.x, objects[0]->face_dir.y};
				fdir *= 20.f;
				pos += fdir;
				populateProjectile(objects.back(), world, sf::Vector2f{pos.x, pos.y}, objects[0]->face_dir);
			}
		}
		
		// let enemies track the player
		b2Vec2 dir;
		for (auto& obj: objects) {
			if (obj->body != nullptr && obj->is_enemy) {
				dir = objects[0]->body->GetPosition() - obj->body->GetPosition();
				dir.Normalize();
				obj->face_dir = {dir.x, dir.y};
				obj->fov.setDirection({dir.x, dir.y});
				dir *= 33.f;
				obj->body->SetLinearVelocity(dir);
			}
		}
		
		// adjust face_dir towards mouse
		dir.SetZero();
		auto mpos = sf::Mouse::getPosition(window);
		auto opos = objects[0]->body->GetPosition();
		dir.x = mpos.x - opos.x;
		dir.y = mpos.y - opos.y;
		dir.Normalize();
		objects[0]->face_dir = {dir.x, dir.y};
		// adjust fov arc
		objects[0]->fov.setDirection({dir.x, dir.y});
		
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
			auto angle = getAngle({objects[0]->face_dir.x, objects[0]->face_dir.y});
			b2Rot r;
			r.Set(-angle); // note: -x due to rotation sense of direction
			dir = b2Mul(r, dir);
		}
		
		// determine whether player is moving backwards
		auto angle = getAngle({objects[0]->face_dir.x, objects[0]->face_dir.y}, dir) * 180 / 3.14;
		if (abs(angle) > 135.f) {
			// backwards!
			dir *= 0.5f;
		} else if (abs(angle) > 45.f) {
			// sidewards!
			dir *= 0.66f;
		}
		
		// move it!
		dir *= 100.f;
		objects[0]->body->SetLinearVelocity(dir);
		
		// simulate world
		world.Step(1/60.f, 8, 3);
		
		cleanup.clear();
		bullets.clear();
		for (auto it = objects.begin(); it != objects.end(); ++it) {
			auto obj = it->get();
			// respawn to center if requested
			if (obj->respawn) {
				obj->body->SetTransform({400, 300}, obj->body->GetAngle());
				obj->respawn = false;
			}
			// delete if marked as drop
			if (obj->drop) {
				world.DestroyBody(obj->body);
				obj->body = nullptr;
				obj->drop = false;
				cleanup.push_back(it);
			}
			if (obj->body != nullptr && obj->is_projectile) {			
				auto pos = obj->body->GetPosition();
				bullets.append({{pos.x, pos.y}, sf::Color::Red});
			}
		}
		for (auto it: cleanup) {
			pop(objects, it, false);
		}
		
		// render scene
		window.clear(sf::Color::Black);
		window.draw(wall_vertices);
		for (auto& obj: objects) {
			if (obj->body == nullptr) {
				continue;
			}
			auto pos = obj->body->GetPosition();
			if (!obj->is_projectile) {
				// note: I just use one Fixture per GameObject's Body, hence the List contains only the first
				// in production, the rendering systems already knows the exact rendering shape
				if (obj->body->GetFixtureList()->GetType() == b2Shape::e_circle) {
					renderCirc(window, {pos.x, pos.y}, obj->radius);
				} else {
					renderRect(window, {pos.x, pos.y}, obj->size);
				}
			
				// draw face_dir
				if (obj->face_dir.x != 0 || obj->face_dir.y != 0) {
					sf::VertexArray vertices{sf::Lines, 2};
					vertices[0].position = {pos.x, pos.y};
					vertices[0].color = sf::Color::Red;
					vertices[1].position = vertices[0].position + sf::Vector2f{obj->face_dir} * 15.f;
					vertices[1].color = sf::Color::Yellow;
					window.draw(vertices);
					
					obj->fov.setPosition({pos.x, pos.y});
					window.draw(obj->fov);
				}
			}
		}
		
		// draw bullets
		window.draw(bullets);
		
		++frames;
		auto elapsed = clock.restart();
		time += elapsed.asMilliseconds();
		if (time >= 1000u) {
			auto s = "FPS: " + std::to_string(frames);
			window.setTitle("Box2D + SFML Demo [" + s + "]");
			time -= 1000u;
			frames = 0u;
		}
		
		window.display();
	}
}

