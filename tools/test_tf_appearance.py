"""Test actual server appearance functions and every generated model/skin."""
from pathlib import Path
import hashlib
import json
import re

import build_tf_appearance as assets
import test_spy_disguise as spy

ROOT = Path(__file__).resolve().parents[1]
text = (ROOT / "src/tfort.c").read_text(encoding="utf-8")
get_skin = text[text.index("const char* TeamFortress_GetSkin( "):text.index("void TeamFortress_SetSkin(")]
set_skin = text[text.index("void TeamFortress_SetSkin("):text.index("void TeamFortress_SetColor(")]
appearance = text[text.index("void TeamFortress_SetColor("):text.index("static void setArmorType(")]
prefix = spy.prefix.replace("int playerclass,isSpectator", "int isBot; float invisible_finished; int playerclass,isSpectator")
prefix = prefix.replace("char *classname;", "float mins[3], maxs[3], origin[3], angles[3], velocity[3], flags, movetype; char *model; char *classname;")
prefix = prefix.replace("skin,team,nextthink;", "skin,team,nextthink,colormap;")
prefix = prefix.replace("void TeamFortress_SetSkin(gedict_t *p) {}", "")
prefix = prefix.replace("void TeamFortress_SetColor(gedict_t *p,int a,int b) {}", "")
prefix = prefix.replace("int TeamFortress_TeamGetTopColor(int t) {return t;}", "int colors[]={0,13,4,12,11}; int TeamFortress_TeamGetTopColor(int t) {return colors[t];}")
prefix = prefix.replace("int TeamFortress_TeamGetColor(int t) {return t;}", "int TeamFortress_TeamGetColor(int t) {return colors[t]+1;}")
extra = r'''
typedef float vec3_t[3];
int modelindex_player=1, modelindex_disguise[5]={0,2,3,4,5};
#define PC_CIVILIAN 11
#define _snprintf snprintf
#define VectorCopy(a,b) memcpy(b,a,sizeof(float)*3)
#define PASSVEC3(a) (a)[0],(a)[1],(a)[2]
void setmodel(gedict_t *p,char *model) {
 p->s.v.model=model;
}
void setorigin(gedict_t *p,float a,float b,float c) {p->s.v.origin[0]=a;p->s.v.origin[1]=b;p->s.v.origin[2]=c;}
void setsize(gedict_t *p,float a,float b,float c,float d,float e,float f) {
 p->s.v.mins[0]=a;p->s.v.mins[1]=b;p->s.v.mins[2]=c;
 p->s.v.maxs[0]=d;p->s.v.maxs[1]=e;p->s.v.maxs[2]=f;
}
const char *TeamFortress_GetSkinByTeamClass(int team,int pc) {return "ordinary_class_skin";}
int NUM_FOR_EDICT(gedict_t *p) {return p-entities;}
void stuffcmd(gedict_t *p,const char *fmt,...) {
 va_list ap;va_start(ap,fmt);vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap);va_end(ap);
}
void trap_SetBotUserInfo(int ent,char *key,const char *value,int flags) {
 stuffcmd(&entities[ent],"%s=%s\n",key,value);
}
#define MAX_BODYQUE 4
static gedict_t *bodyque[4]={&entities[3],&entities[4],&entities[5],&entities[6]};
static int bodyque_head;
'''
tests = r'''
int main(void) {
 int team,pc; char expected[64]; gedict_t *p=&entities[1],*body=&entities[2];
 for(team=1;team<=4;team++) for(pc=1;pc<=9;pc++) {
  reset(); p->undercover_team=team; p->undercover_skin=pc;
  assert(!strcmp(TeamFortress_GetSkin(p),"ordinary_class_skin"));
  TeamFortress_SetSkin(p);
  snprintf(expected,sizeof(expected),"progs/tfbody%d.mdl",team);
  assert(!strcmp(p->s.v.model,expected) && p->s.v.modelindex==modelindex_disguise[team]);
  assert(p->s.v.skin==pc);
  assert(!strstr(output,"tf_dc")); /* no new runtime PCX dependency */
  output[0]=0;
  TeamFortress_SetColor(p,colors[team],colors[team]);
  assert(!strcmp(output,"color 13 13\n")); /* always real blue team */
  setsize(body,-16,-16,-24,16,16,32);
  TeamFortress_SetCorpseAppearance(body,p,0);
  snprintf(expected,sizeof(expected),"progs/tfbody%d.mdl",team);
  assert(!strcmp(body->s.v.model,expected) && body->s.v.skin==pc && !body->s.v.colormap);
  assert(body->s.v.mins[2]==-24 && body->s.v.maxs[2]==32);
  p->undercover_team=0;p->undercover_skin=0;p->playerclass=PC_MEDIC;
  TeamFortress_SetSkin(p); /* owner respawns, then changes class/colors */
  assert(!strcmp(body->s.v.model,expected) && body->s.v.skin==pc && !body->s.v.colormap);
 }
 reset(); p->isBot=1;p->undercover_team=2;
 TeamFortress_SetColor(p,4,4);assert(!strcmp(output,"topcolor=13\nbottomcolor=13\n"));
 reset(); p->playerclass=PC_SOLDIER;p->team_no=2;
 TeamFortress_SetCorpseAppearance(body,p,0);assert(body->s.v.skin==PC_SOLDIER && !strcmp(body->s.v.model,"progs/tfbody2.mdl"));
 TeamFortress_SetCorpseAppearance(body,p,1);assert(body->s.v.skin==PC_SOLDIER && !strcmp(body->s.v.model,"progs/tfheadless2.mdl"));
 reset();p->undercover_team=2;p->undercover_skin=PC_SOLDIER;p->s.v.deadflag=1;
 TeamFortress_SetCorpseAppearance(p,p,0);p->undercover_team=0;p->undercover_skin=0;
 TeamFortress_SetSkin(p);assert(p->s.v.skin==PC_SOLDIER && !strcmp(p->s.v.model,"progs/tfbody2.mdl"));
 bodyque_head=0;CopyToBodyQue(p);
 p->s.v.deadflag=0;p->playerclass=PC_MEDIC;p->team_no=1;
 setmodel(p,"progs/player.mdl");p->s.v.colormap=1;TeamFortress_SetSkin(p);
 assert(!strcmp(bodyque[0]->s.v.model,"progs/tfbody2.mdl") && bodyque[0]->s.v.skin==PC_SOLDIER && !bodyque[0]->s.v.colormap);
 reset();p->undercover_team=2;p->undercover_skin=PC_SOLDIER;
 p->s.v.modelindex=99;p->invisible_finished=200;TeamFortress_SetSkin(p);
 assert(p->s.v.modelindex==99); /* ring/Spy invisibility is preserved */
 p->invisible_finished=0;TeamFortress_SetPlayerAppearance(p);
 assert(p->s.v.modelindex==modelindex_disguise[2] && p->s.v.skin==PC_SOLDIER);
 p->undercover_team=0;p->undercover_skin=0;TeamFortress_SetSkin(p);
 assert(p->s.v.modelindex==modelindex_player && p->s.v.skin==PC_SPY && !strcmp(p->s.v.model,"progs/player.mdl"));
 reset();run("disguise_sold_color");
 assert(p->s.v.modelindex==modelindex_player);think(2,104);
 assert(p->s.v.skin==PC_SOLDIER && p->s.v.modelindex==modelindex_player);
 think(2,108);assert(p->s.v.skin==PC_SOLDIER && p->s.v.modelindex==modelindex_disguise[2]);
 assert(!strcmp(p->s.v.model,"progs/tfbody2.mdl") && !strstr(output,"tf_dc"));
 reset();run("disguise_color");think(2,104);
 assert(p->s.v.skin==PC_SPY && p->s.v.modelindex==modelindex_disguise[2]);
 puts("PASS: real scoreboard colors for humans/bots; 36 Spy disguises; immutable corpse class/color/hull; headless bodies; dying Spy reset");
 return 0;
}
'''
world = (ROOT / "src/world.c").read_text(encoding="utf-8")
copy_body = world[world.index("void CopyToBodyQue("):world.index("//=======================", world.index("void CopyToBodyQue("))]
spy.compile_and_run(prefix + extra + get_skin + set_skin + appearance + spy.functions + copy_body + tests)

overlay = ROOT / "assets/fortress"
manifest = json.loads((overlay / "appearance-manifest.json").read_text())
for name, info in manifest["files"].items():
    data = (overlay / name).read_bytes()
    assert hashlib.sha256(data).hexdigest() == info["sha256"]
    if name.endswith(".pcx"):
        pixels = assets.decode_pcx(data)[2]
        assert not any(16 <= p < 32 or 96 <= p < 112 for p in pixels)
    else:
        header, width, height, skins, geometry = assets.split_mdl(data)
        assert len(skins) == 10 and geometry
        for pixels in skins:
            assert not any(16 <= p < 32 or 96 <= p < 112 for p in pixels)
            assert len(pixels) == width * height
# Both validation paths must check the true team even while disguised.
for filename in ["g_cmd.c", "tforttm.c"]:
    code = (ROOT / "src" / filename).read_text(encoding="utf-8")
    assert not re.search(r"TeamFortress_TeamGet(?:Top)?Color\( self->undercover_team \)", code)
print("PASS: all 44 resource hashes, locked palette ramps, model skin counts and color validation paths")
client = (ROOT / "src/client.c").read_text(encoding="utf-8")
powerups = client[client.index("void CheckPowerups()") : client.index("void CheckPowerups()") + 3800]
assert "TeamFortress_SetPlayerAppearance( self )" in powerups
assert "self->s.v.modelindex = modelindex_player" not in powerups
print("PASS: live disguise model selection with no runtime PCX; invisibility, reset and per-frame restoration")
