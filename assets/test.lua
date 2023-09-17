function foo()
    local a = Ecs:createEntity()
print("A:", a)
    local c = EcsType.Component
    print("C:", c)
    local b = Ecs:createEntity(c)
    print("B", b)
end

foo()
