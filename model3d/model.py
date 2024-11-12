import cadquery as cq

# Dimensions of the whole panel
panel_length = 160.0
panel_width = 160.0
panel_height = 0.2 # idk/wild guess/thereabouts
panel_leds_cols = 16
panel_leds_rows = 16

# Dimensions of the LED module
## Box
led_side = 5.0
led_height = 1.57
## Bounding box
led_box_side = panel_length / panel_leds_cols
## Inward cone
led_hole_dia1 = 4.0
led_hole_dia2 = 3.35
led_hole_depth = 0.8

# Light diffusor attributes
tolerance = 0.1 # how much space to led module
line_width = 0.4 # default for 0.4mm nozzle
number_of_lines = 2 # number of walls in the diffusor
line_height = 0.2 # default in Cura for normal quality
skin_lines = 2 # number of layers within the diffusor
wall_width = line_width * number_of_lines # easier for the eyes
diffusor_height = led_box_side/2 - tolerance - wall_width - led_side/2 # some trigonometry to achieve 45 deg angle
skin_height = line_height * skin_lines # easier for the eyes

# Create the backbone of the panel
led_panel = (
    cq.Workplane("XY")
    .rect(panel_length, panel_width, centered=False)
    .extrude(-panel_height)
)

# Create a single LED based on dimensions
led_single = (
    cq.Workplane("XY")
    .transformed(offset=(panel_length/panel_leds_cols/2, 
                         panel_width/panel_leds_rows/2,
                         0.0))
    .rect(led_side, led_side)
    .extrude(led_height)
    .faces(">Z")
    .circle(led_hole_dia1/2)
    .workplane(offset=-led_hole_depth)
    .circle(led_hole_dia2/2)
    .loft(combine="cut")
)

# Create a single diffusor
diffusor_single = (
    cq.Workplane("XY")
    .transformed(offset=(panel_length/panel_leds_cols/2, 
                         panel_width/panel_leds_rows/2,
                         0.0))
    .rect(led_side+2*tolerance+2*wall_width, 
          led_side+2*tolerance+2*wall_width)
    .workplane(offset=led_height)
    .rect(led_side+2*tolerance+2*wall_width, 
          led_side+2*tolerance+2*wall_width)
    .workplane(offset=diffusor_height)
    .rect(led_box_side, 
          led_box_side)
    .loft(ruled=True)
    .faces("|Z")
    .shell(-wall_width)
    .edges("((|Y or |X) and <Z)")
    .edges("<<Y[1] or <<Y[3] or <<X[1] or <<X[3]")
    .chamfer(wall_width/2)
)

# Create frontplate
frontplate = (
    cq.Workplane("XY")
    .transformed(offset=(-wall_width, 
                         -wall_width,
                         led_height+diffusor_height))
    .rect(panel_length+wall_width*2, panel_width+wall_width*2, centered=False)
    .extrude(skin_height)
    .rect(panel_length, panel_width)
    .rect(panel_length+wall_width*2, panel_width+wall_width*2)
    .extrude(-(skin_height+diffusor_height+led_height+panel_height+5))
)
# Populate the panel with LEDs and diffusors
# LEDs commented out because of low performance - still a valid code for testing

for col in range(panel_leds_cols):
    for row in range(panel_leds_rows):
#        led_panel = led_panel.union(led_single.translate((panel_length/panel_leds_rows*row, 
#                                                          panel_width/panel_leds_cols*col,
#                                                          0.0)))
        frontplate = frontplate.union(diffusor_single.translate((panel_length/panel_leds_rows*row, 
                                                                 panel_width/panel_leds_cols*col,
                                                                 0.0)))

frontplate.export("./LED_Panel_Diffusor.stl")

result = frontplate

show_object(result)