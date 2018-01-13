#include <iostream>
#include <memory>
#include <iostream>
#include <ctime>

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

int main() {
    std::srand(std::time(nullptr));

    // setup world with custom collision handling
    b2Vec2 gravity{0.f, 0.f};
    b2World world{gravity};
    CollisionHandler handler;
    world.SetContactListener(&handler);
    
    // create some objects
    auto actor = createCirc(world, {400.f, 300.f}, true, 10.f);
    auto data = std::make_unique<int>(42); // just a demo for userdata access
    actor->SetUserData(data.get());
    
    auto second = createCirc(world, {200.f, 400.f}, true, 10.f);
    auto third  = createCirc(world, {700.f, 350.f}, true, 10.f);
    
    // create random blocks
    for (int i = 0; i < 25; ++i) {
        float x = std::rand() % 700 + 50;
        float y = std::rand() % 500 + 50;
        createRect(world, {x, y}, false, {20.f, 20.f});
    }
    
    // create tile border
    // todo
    
    sf::RenderWindow window{sf::VideoMode{800u, 600u}, "Box2D + SFML Demo"};
    
    while (window.isOpen()) {
        // input stuff
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
        
        // let second and third object track the player
        auto dir = actor->GetPosition() - second->GetPosition();
        dir.Normalize();
        dir *= 33.f;
        second->SetLinearVelocity(dir);
        
        dir = actor->GetPosition() - third->GetPosition();
        dir.Normalize();
        dir *= 33.f;
        third->SetLinearVelocity(dir);
        
        // apply direction of player according to arrow keys
        dir.SetZero();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    { dir.y = -1.f; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  { dir.y =  1.f; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  { dir.x = -1.f; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) { dir.x =  1.f; }
        dir.Normalize();
        dir *= 100.f;
        actor->SetLinearVelocity(dir);
        
        // simulate world
        world.Step(1/60.f, 8, 3);
        
        // render scene
        window.clear(sf::Color::Black);
        for (b2Body* it = world.GetBodyList(); it != 0; it = it->GetNext()) {
            auto pos = it->GetPosition();
            // note: I just use one Fixture per Body, hence the List contains only the first
            // in production, the rendering systems already knows the exact rendering shape
            if (it->GetFixtureList()->GetType() == b2Shape::e_circle) {
                renderCirc(window, {pos.x, pos.y});
            } else {
                renderRect(window, {pos.x, pos.y});
            }
        }
        window.display();
    }
}

