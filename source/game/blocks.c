#include "game/game_main.h"

const blockTex_struct blockTextures[]={(blockTex_struct){2,0},
									(blockTex_struct){3,0},
									(blockTex_struct){0,0},
									(blockTex_struct){1,0},
									(blockTex_struct){0,1},
									(blockTex_struct){1,1},
									(blockTex_struct){2,1},
									(blockTex_struct){4,0},
									(blockTex_struct){4,1},
									(blockTex_struct){5,1},
									(blockTex_struct){5,3},
									(blockTex_struct){13,12},
									(blockTex_struct){1,3},
									(blockTex_struct){0,5},
									(blockTex_struct){1,7},
									(blockTex_struct){1,8},
									(blockTex_struct){1,9},
									(blockTex_struct){1,10},
									(blockTex_struct){1,11},
									(blockTex_struct){1,12},
									(blockTex_struct){1,13},
									(blockTex_struct){1,14},
									(blockTex_struct){2,7},
									(blockTex_struct){2,8},
									(blockTex_struct){2,9},
									(blockTex_struct){2,10},
									(blockTex_struct){2,11},
									(blockTex_struct){2,12},
									(blockTex_struct){2,13},
									(blockTex_struct){4,7},
									(blockTex_struct){5,7},
									(blockTex_struct){6,1},
									(blockTex_struct){7,1},
									(blockTex_struct){8,1},
									(blockTex_struct){0,9},
									(blockTex_struct){4,2},
									(blockTex_struct){5,2},
									(blockTex_struct){7,6},
									(blockTex_struct){8,6},
									(blockTex_struct){9,6},
									(blockTex_struct){3,5},
									(blockTex_struct){1,5},
									(blockTex_struct){1,6},
									(blockTex_struct){0,0},//USED
									(blockTex_struct){0,0},//USED
									(blockTex_struct){11,2},//crafting table top
									(blockTex_struct){11,3},//crafting table side
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){2,2},//coal ore
									(blockTex_struct){0,0},
									(blockTex_struct){9,1},//chest top
									(blockTex_struct){10,1},//chest side
									(blockTex_struct){11,1},//chest front
									(blockTex_struct){9,2},//double chest front left
									(blockTex_struct){10,2},//double chest front right
									(blockTex_struct){9,3},//double chest back left
									(blockTex_struct){10,3},//double chest back right
									(blockTex_struct){12,2},//furnace front
									(blockTex_struct){13,2},//furnace side
									(blockTex_struct){14,3},//furnace top
									(blockTex_struct){13,3},//furnace front (lit)
									(blockTex_struct){0,0},//charcoal (items.png)
									(blockTex_struct){15,0},//sapling
									(blockTex_struct){0,0},//apple (items.png)
									(blockTex_struct){6,5},//farmland (wet)
									(blockTex_struct){7,5},//farmland (dry)
									(blockTex_struct){6,6},//pumpkin top
									(blockTex_struct){6,7},//pumpkin side
									(blockTex_struct){7,7},//pumpkin face
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){0,0},
									(blockTex_struct){11,3},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12},
									(blockTex_struct){13,12}};
									
const block_struct blocks[]={(block_struct){0,0,0},
							(block_struct){2,1,1},
							(block_struct){0,0,0},
							(block_struct){3,3,3},
							(block_struct){4,4,4},
							(block_struct){5,5,5},
							(block_struct){6,6,6},
							(block_struct){7,7,7},
							(block_struct){9,8,9},
							(block_struct){9,9,9},
							(block_struct){10,10,10},
							(block_struct){11,11,11},
							(block_struct){12,12,12},
							(block_struct){13,13,13},
							(block_struct){14,14,14},
							(block_struct){15,15,15},
							(block_struct){16,16,16},
							(block_struct){17,17,17},
							(block_struct){18,18,18},
							(block_struct){19,19,19},
							(block_struct){20,20,20},
							(block_struct){21,21,21},
							(block_struct){22,22,22},
							(block_struct){23,23,23},
							(block_struct){24,24,24},
							(block_struct){25,25,25},
							(block_struct){26,26,26},
							(block_struct){27,27,27},
							(block_struct){28,28,28},
							(block_struct){9,29,9},
							(block_struct){9,30,9},
							(block_struct){31,31,31},
							(block_struct){32,32,32},
							(block_struct){33,33,33},
							(block_struct){34,34,34},
							(block_struct){35,35,35},
							(block_struct){36,36,36},
							(block_struct){37,37,37},
							(block_struct){38,38,38},
							(block_struct){39,39,39},
							(block_struct){40,40,40},
							(block_struct){40,40,40},
							(block_struct){40,40,40},
							(block_struct){40,40,40},
							(block_struct){41,41,44},
							(block_struct){42,42,44},
							(block_struct){41,41,44},
							(block_struct){42,42,44},
							(block_struct){41,41,44},
							(block_struct){42,42,44},
							(block_struct){41,41,44},
							(block_struct){42,42,44},
							(block_struct){41,41,44},
							(block_struct){42,42,44},
							(block_struct){41,41,44},
							(block_struct){42,42,44},
							(block_struct){41,41,44},
							(block_struct){42,42,44},
							(block_struct){41,41,44},
							(block_struct){42,42,44},
							(block_struct){45,7,46},//crafting table
							(block_struct){54,54,54},//coal ore
							(block_struct){56,56,58},//chest (item icon)
							(block_struct){65,65,63},//furnace (item)
							(block_struct){47,47,47},//item texture
							(block_struct){48,48,48},//item texture
							(block_struct){49,49,49},//item texture
							(block_struct){50,50,50},//item texture
							(block_struct){51,51,51},//item texture
							(block_struct){52,52,52},//item texture
							(block_struct){53,53,53},//item texture
							(block_struct){55,55,55},//coal (item texture)
							(block_struct){56,56,57},//placed chest (faces from chestTexture)
							(block_struct){56,56,57},//placed chest (faces from chestTexture)
							(block_struct){56,56,57},//placed chest (faces from chestTexture)
							(block_struct){56,56,57},//placed chest (faces from chestTexture)
							(block_struct){56,56,57},//placed chest (faces from chestTexture)
							(block_struct){56,56,57},//placed chest (faces from chestTexture)
							(block_struct){56,56,57},//placed chest (faces from chestTexture)
							(block_struct){56,56,57},//placed chest (faces from chestTexture)
							(block_struct){56,56,57},//placed chest (faces from chestTexture)
							(block_struct){56,56,57},//placed chest (faces from chestTexture)
							(block_struct){56,56,57},//placed chest (faces from chestTexture)
							(block_struct){56,56,57},//placed chest (faces from chestTexture)
							(block_struct){65,65,64},//placed furnace (faces from furnaceTexture)
							(block_struct){65,65,64},//placed furnace (faces from furnaceTexture)
							(block_struct){65,65,64},//placed furnace (faces from furnaceTexture)
							(block_struct){65,65,64},//placed furnace (faces from furnaceTexture)
							(block_struct){65,65,64},//placed furnace (faces from furnaceTexture)
							(block_struct){65,65,64},//placed furnace (faces from furnaceTexture)
							(block_struct){65,65,64},//placed furnace (faces from furnaceTexture)
							(block_struct){65,65,64},//placed furnace (faces from furnaceTexture)
							(block_struct){67,67,67},//charcoal (item texture)
							(block_struct){10,10,10},//leaves marked for decay
							(block_struct){10,10,10},//leaves placed by the player
							(block_struct){68,68,68},//sapling
							(block_struct){68,68,68},//sapling (grown stage)
							(block_struct){69,69,69},//apple (item texture)
							(block_struct){89,89,89},//carrot (item)
							(block_struct){90,90,90},//pumpkin seeds
							(block_struct){91,91,91},//wooden hoe
							(block_struct){92,92,92},//stone hoe
							(block_struct){72,72,74},//pumpkin (item)
							(block_struct){98,98,98},//iron ore
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){97,97,97},//iron ingot (item texture)
							(block_struct){93,93,93},//iron pickaxe
							(block_struct){94,94,94},//iron shovel
							(block_struct){95,95,95},//iron axe
							(block_struct){96,96,96},//iron hoe
							(block_struct){0,0,0},
							(block_struct){71,0,0},//farmland (dry)
							(block_struct){70,0,0},//farmland (wet)
							(block_struct){70,0,0},//farmland (wet)
							(block_struct){70,0,0},//farmland (wet)
							(block_struct){70,0,0},//farmland (wet)
							(block_struct){70,0,0},//farmland (wet)
							(block_struct){70,0,0},//farmland (wet)
							(block_struct){70,0,0},//farmland (wet)
							(block_struct){75,75,75},//carrots (faces from farmTexture)
							(block_struct){75,75,75},//carrots (faces from farmTexture)
							(block_struct){75,75,75},//carrots (faces from farmTexture)
							(block_struct){75,75,75},//carrots (faces from farmTexture)
							(block_struct){75,75,75},//carrots (faces from farmTexture)
							(block_struct){75,75,75},//carrots (faces from farmTexture)
							(block_struct){75,75,75},//carrots (faces from farmTexture)
							(block_struct){75,75,75},//carrots (faces from farmTexture)
							(block_struct){79,79,79},//pumpkin stem (faces from farmTexture)
							(block_struct){79,79,79},//pumpkin stem (faces from farmTexture)
							(block_struct){79,79,79},//pumpkin stem (faces from farmTexture)
							(block_struct){79,79,79},//pumpkin stem (faces from farmTexture)
							(block_struct){79,79,79},//pumpkin stem (faces from farmTexture)
							(block_struct){79,79,79},//pumpkin stem (faces from farmTexture)
							(block_struct){79,79,79},//pumpkin stem (faces from farmTexture)
							(block_struct){79,79,79},//pumpkin stem (faces from farmTexture)
							(block_struct){79,79,79},//pumpkin stem (faces from farmTexture)
							(block_struct){79,79,79},//pumpkin stem (faces from farmTexture)
							(block_struct){79,79,79},//pumpkin stem (faces from farmTexture)
							(block_struct){79,79,79},//pumpkin stem (faces from farmTexture)
							(block_struct){72,72,73},//pumpkin (faces from farmTexture)
							(block_struct){72,72,73},//pumpkin (faces from farmTexture)
							(block_struct){72,72,73},//pumpkin (faces from farmTexture)
							(block_struct){72,72,73},//pumpkin (faces from farmTexture)
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){0,0,0},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE},
							(block_struct){WATERTYPE,WATERTYPE,WATERTYPE}};