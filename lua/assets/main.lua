io.write("Begin of main.lua\n");

local assets_dir = sdl.assets_dir()
io.write(assets_dir .. "\n")

local image_filename = assets_dir .. "wabbit_alpha.png"
local img = image.load(image_filename)
io.write("image: " .. tostring(img) .. "\n")
local tex = texture.from_image(img)
io.write("tex: " .. tostring(tex) .. "\n")

--local shd = shader.load_from_file(assets_dir .. "default.vert", assets_dir .. "default.frag")
--io.write("shader: " .. tostring(shd) .. "\n")
local mat = material.new()

material.set_texture(mat, tex)
--material.set_shader(mat, shd)
io.write("material: " .. tostring(mat) .. "\n")

local player = sprite.from_material(mat)
if player == nil or player == nullptr then
    io.write("player is nil")
else
    io.write("player: " .. tostring(player) .. "\n")
end

local player_x = 100
local player_y = 100

local scale = lkazmath.kmVec2New()
scale.x = 1.0
scale.y = 1.0
io.write("scale: " .. tostring(scale) .. "\n")
io.write("scale.x: " .. tostring(scale.x) .. "\n")
io.write("scale.y: " .. tostring(scale.y) .. "\n")

io.write("gd: " .. tostring(gd) .. "\n")
io.write("viewport: " .. tostring(viewport) .. "\n")
io.write("camera: " .. tostring(camera) .. "\n")

function on_update(dt)
    --[[
    io.write(dt)
    io.write("player: " .. tostring(player) .. "\n")
    io.write("gd: " .. tostring(gd) .. "\n")
    io.write("viewport: " .. tostring(viewport) .. "\n")
    io.write("scale: " .. tostring(scale) .. "\n")
    io.write("camera: " .. tostring(camera) .. "\n")
    ]]
    --io.write(tostring(scale.y))

    if input.is_key_pressed(input_mgr, key.KEY_RIGHT) then
        player_x = player_x + 100 * dt
    elseif input.is_key_pressed(input_mgr, key.KEY_LEFT) then
        player_x = player_x - 100 * dt
    end

    if input.is_key_pressed(input_mgr, key.KEY_UP) then
        player_y = player_y + 100 * dt
    elseif input.is_key_pressed(input_mgr, key.KEY_DOWN) then
        player_y = player_y - 100 * dt
    end

    sprite.draw(player, gd, player_x, player_y, viewport, 0, scale, camera_mgr)
end

io.write("End of main.lua\n");