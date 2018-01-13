#include <iostream>

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>

// physics --------------------------------------------------------------------

b2Body* createBody(b2World& world, sf::Vector2f const & pos, bool dynamic, b2Shape* shape) {
    b2FixtureDef fixdef;
    fixdef.shape = shape;
    
    b2BodyDef bodydef;
    bodydef.position = b2Vec2(pos.x, pos.y);
    bodydef.type = dynamic ? b2_dynamicBody : b2_staticBody;
    
    auto body = world.CreateBody(&bodydef);
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

void setMovement(b2Body* body, sf::Vector2f const & towards) {
    auto p = body->GetPosition();
    sf::Vector2f diff{towards.x - p.x, towards.y - p.y};
    auto n = std::sqrt(diff.x * diff.x + diff.y * diff.y);
    diff.x /= n;
    diff.y /= n;
    body->SetLinearVelocity(b2Vec2(diff.x, diff.y));
}

class CollisionHandler: public b2ContactListener {
    public:
        void BeginContact(b2Contact* contact);
        void EndContact(b2Contact* contact);
        void PreSolve(b2Contact* contact, b2Manifold const * oldManifold);
        void PostSolve(b2Contact* contact, b2ContactImpulse const * impulse);
};

void CollisionHandler::BeginContact(b2Contact* contact) {
    auto a = contact->GetFixtureA()->GetBody();
    auto b = contact->GetFixtureB()->GetBody();
    std::cout << "Begin " << a << " vs. " << b << "\n";
}

void CollisionHandler::EndContact(b2Contact* contact) {
    auto a = contact->GetFixtureA()->GetBody();
    auto b = contact->GetFixtureB()->GetBody();
    std::cout << "End " << a << " vs. " << b << "\n";
}

void CollisionHandler::PreSolve(b2Contact* contact, b2Manifold const * oldManifold) {
}

void CollisionHandler::PostSolve(b2Contact* contact, b2ContactImpulse const * impulse) {
}


// rendering ------------------------------------------------------------------

void renderCirc(sf::RenderTarget& target, sf::Vector2f const & pos) {
    sf::CircleShape circ{10.f};
    circ.setOrigin({circ.getRadius(), circ.getRadius()});
    circ.setPosition(pos);
    circ.setOutlineColor(sf::Color::White);
    circ.setFillColor(sf::Color::Transparent);
    circ.setOutlineThickness(1.f);
    target.draw(circ);
}

void renderRect(sf::RenderTarget& target, sf::Vector2f const & pos) {
    sf::RectangleShape rect{{20.f, 8.f}};
    rect.setOrigin(rect.getSize() / 2.f);
    rect.setPosition(pos);
    rect.setOutlineColor(sf::Color::White);
    rect.setFillColor(sf::Color::Transparent);
    rect.setOutlineThickness(1.f);
    target.draw(rect);
}

// main -----------------------------------------------------------------------

int main() {
    b2Vec2 gravity{0.f, 0.f};
    b2World world{gravity};
    CollisionHandler handler;
    world.SetContactListener(&handler);
    
    auto actor = createCirc(world, {400.f, 300.f}, true, 10.f);
    
    auto second = createCirc(world, {200.f, 400.f}, true, 10.f);
    second->SetLinearVelocity(b2Vec2(5.f, 0.f));
    
    createRect(world, {400.f, 400.f}, false, {20.f, 8.f});
    
    sf::RenderWindow window{sf::VideoMode{800u, 600u}, "Box2D + SFML Demo"};
    
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) {
                    // create fixed body on space key
                    sf::Vector2f pos{sf::Mouse::getPosition(window)};
                    createRect(world, pos, false, {20.f, 8.f});
                }
            }
        }
        
        // apply director according to arrow keys
        sf::Vector2f dir{0.f, 0.f};
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    { dir.y = -1.f; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  { dir.y =  1.f; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  { dir.x = -1.f; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) { dir.x =  1.f; }
        dir *= 5.f;
        actor->SetLinearVelocity(b2Vec2(dir.x, dir.y));
        
        // simulate world
        world.Step(1/60.f, 8, 3);
        
        // render scene
        window.clear(sf::Color::Black);
        for (b2Body* it = world.GetBodyList(); it != 0; it = it->GetNext()) {
            auto pos = it->GetPosition();
            if (it->GetType() == b2_dynamicBody) {
                renderCirc(window, {pos.x, pos.y});
            } else {
                renderRect(window, {pos.x, pos.y});
            }
        }
        window.display();
    }
}

