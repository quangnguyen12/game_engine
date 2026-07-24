-- =============================================
--  Patrol.lua
--  Object di chuyển qua lại giữa 2 điểm (tuần tra)
-- =============================================

local Patrol = {}

function Patrol:onStart(obj)
    self.pointA = vec3.new(obj.position.x - 2.0, obj.position.y, obj.position.z)
    self.pointB = vec3.new(obj.position.x + 2.0, obj.position.y, obj.position.z)
    self.speed = 1.5
    self.goingToB = true
    print("[Lua] Patrol started: " .. obj.name)
end

function Patrol:onUpdate(obj, dt)
    local target
    if self.goingToB then
        target = self.pointB
    else
        target = self.pointA
    end

    -- Tính hướng di chuyển
    local dx = target.x - obj.position.x
    local dz = target.z - obj.position.z
    local dist = math.sqrt(dx * dx + dz * dz)

    if dist < 0.1 then
        -- Đã đến đích, đổi hướng
        self.goingToB = not self.goingToB
    else
        -- Di chuyển về phía target
        local nx = dx / dist
        local nz = dz / dist
        obj.position.x = obj.position.x + nx * self.speed * dt
        obj.position.z = obj.position.z + nz * self.speed * dt

        -- Quay mặt theo hướng di chuyển
        obj.rotation.y = math.deg(math.atan(nx, nz))
    end
end

return Patrol
