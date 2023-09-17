local a = Ecs:createEntity()
print("A:", a)
local c = EcsType.Component
print("C:", c)
local b = Ecs:createEntity(c)
print("B", b)
local d = Ecs:createComponent("A")
print("D", d)
