function foo()
    local world = Ecs.new()
    print(world:createEntity())
    print(world:createEntity())
    print(world:createEntity())
    print(world:createEntity())
    print(world:createEntity())
end

foo()
