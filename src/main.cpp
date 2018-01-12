#include <iostream>

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>

struct Object {
//    b2Body
};

//std::vector<

// physics --------------------------------------------------------------------

b2Body* createBody(b2World& world, sf::Vector2f const & pos, float radius) {
    b2BodyDef bodydef;
    bodydef.position = b2Vec2(pos.x, pos.y);
    bodydef.type = b2_dynamicBody;
    b2Body* body = world.CreateBody(&bodydef);
    
    b2CircleShape shape;
    shape.m_radius = radius;
    b2FixtureDef fixdef;
    fixdef.density = 1.f;
    fixdef.friction = 0.7f;
    fixdef.shape = &shape;
    body->CreateFixture(&fixdef);
    
    return body;
}

b2Body* createTile(b2World& world, sf::Vector2f const & pos, sf::Vector2f const & size) {
    b2BodyDef bodydef;
    bodydef.position = b2Vec2(pos.x, pos.y);
    bodydef.type = b2_staticBody;
    b2Body* body = world.CreateBody(&bodydef);
    
    b2PolygonShape shape;
    shape.SetAsBox(size.x/2.f, size.y/2.f);
    b2FixtureDef fixdef;
    fixdef.density = 1.f;
    fixdef.friction = 1.f;
    fixdef.shape = &shape;
    body->CreateFixture(&fixdef);
    
    return body;
}

void setMovement(b2Body* body, sf::Vector2f const & towards) {
    auto p = body->GetPosition();
    sf::Vector2f diff{towards.x - p.x, towards.y - p.y};
    auto n = std::sqrt(diff.x * diff.x + diff.y * diff.y);
    diff.x /= n;
    diff.y /= n;
    body->SetLinearVelocity(b2Vec2(diff.x, diff.y));
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
    auto actor = createBody(world, {400.f, 300.f}, 10.f);
    unsigned int id = 42;
    actor->GetFixtureList()->SetUserData(&id); // store ID to identify as actor

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
                    createTile(world, pos, {20.f, 8.f});
                }
            }
        }
        
        // apply director according to arrow keys
        sf::Vector2i dir{0, 0};
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    { dir.y = -1; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  { dir.y =  1; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  { dir.x = -1; }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) { dir.x =  1; }
        actor->SetLinearVelocity(b2Vec2(dir.x, dir.y));
        
        // simulate world
        world.Step(1/60.f, 8, 3);
        
        // query all collisions
        for (b2Contact* c = world.GetContactList(); c; c = c->GetNext()) {
            auto fixa = c->GetFixtureA();
            auto fixb = c->GetFixtureB();
            
            // todo: find a way to detect each collision ONCE
            // --> the collision must be solved so it isn't triggered again
            if (fixa->GetUserData() == &id || fixb->GetUserData() == &id) {
                std::cout << "Ooops\n";
            }
        }
        
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

