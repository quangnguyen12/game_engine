-- =============================================
--  PlayerWASD.lua
--  Điều khiển di chuyển nhân vật bằng phím W A S D
--  Hỗ trợ nhảy phím Space & di chuyển chéo
-- =============================================

local PlayerWASD = {}

function PlayerWASD:onStart(obj)
    self.speed = 4.0
    print("[Lua] PlayerWASD started on: " .. obj.name)
end

function PlayerWASD:onUpdate(obj, dt)
    local moveX = 0.0
    local moveZ = 0.0

    -- Kiểm tra phím bấm W A S D hoặc Phím Mũi Tên
    if Input.isKeyPressed("W") or Input.isKeyPressed("UP") then
        moveZ = moveZ - 1.0
    end
    if Input.isKeyPressed("S") or Input.isKeyPressed("DOWN") then
        moveZ = moveZ + 1.0
    end
    if Input.isKeyPressed("A") or Input.isKeyPressed("LEFT") then
        moveX = moveX - 1.0
    end
    if Input.isKeyPressed("D") or Input.isKeyPressed("RIGHT") then
        moveX = moveX + 1.0
    end

    -- Chuẩn hóa tốc độ khi di chuyển chéo
    if moveX ~= 0.0 and moveZ ~= 0.0 then
        moveX = moveX * 0.7071
        moveZ = moveZ * 0.7071
    end

    -- Cập nhật tọa độ vị trí object
    obj.position.x = obj.position.x + moveX * self.speed * dt
    obj.position.z = obj.position.z + moveZ * self.speed * dt

    -- Xoay nhân vật theo hướng di chuyển
    if moveX ~= 0.0 or moveZ ~= 0.0 then
        local angle = math.deg(math.atan(moveX, -moveZ))
        obj.rotation.y = angle
    end

    -- Nhảy lên khi nhấn phím Space
    if Input.isKeyPressed("SPACE") and obj.position.y <= 0.05 then
        obj.velocity.y = 5.0
    end
end

return PlayerWASD
