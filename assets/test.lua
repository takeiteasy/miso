local a = Ecs:createEntity()
print("A:", a.id)
local c = EcsType.Component
print("C:", c)
local d = Ecs:createComponent("position", {x = 1, y = 2})
print("D:", d.id)
local b = Ecs:createEntity(c)
b:add("position")
print("B:", b.id)
local e = b:get("position")
print("E:")
LuaDumpTable(e)
b:set("position", {x = 0})
local f = b:get(d)
print("F:")
LuaDumpTable(f)
