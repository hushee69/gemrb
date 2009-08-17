/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 *
 */

class Map;

#ifndef MAP_H
#define MAP_H

#include "../../includes/globals.h"
#include "ActorBlock.h"
#include "IniSpawn.h"
#include "SpriteCover.h"
#include <queue>

class Actor;
class TileMap;
class ImageMgr;
class Ambient;
class GameControl;
struct PathNode;
class ScriptedAnimation;
class Animation;
class Wall_Polygon;
class Particles;
class Projectile;
class AnimationFactory;

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

//distance of actors from spawn point
#define SPAWN_RANGE       400

//area flags
#define AF_SAVE           1
#define AF_TUTORIAL       2
#define AF_DEADMAGIC      4
#define AF_DREAM          8

//area types
#define AT_OUTDOOR        1
#define AT_DAYNIGHT       2
#define AT_WEATHER        4
#define AT_CITY           8
#define AT_FOREST         0x10
#define AT_DUNGEON        0x20
#define AT_EXTENDED_NIGHT 0x40
#define AT_CAN_REST       0x80

//area animation flags
#define A_ANI_ACTIVE          1        //if not set, animation is invisible
#define A_ANI_BLEND           2        //blend
#define A_ANI_NO_SHADOW       4        //lightmap doesn't affect it
#define A_ANI_PLAYONCE        8        //stop after endframe
#define A_ANI_SYNC            16       //synchronised draw (skip frames if needed)
#define A_ANI_32              32
#define A_ANI_NO_WALL         64       //draw after walls (walls don't cover it)
#define A_ANI_NOT_IN_FOG      0x80     //not visible in fog of war
#define A_ANI_BACKGROUND      0x100    //draw before actors (actors cover it)
#define A_ANI_ALLCYCLES       0x200    //draw all cycles, not just the cycle specified
#define A_ANI_PALETTE         0x400    //has own palette set
#define A_ANI_MIRROR          0x800    //mirrored
#define A_ANI_COMBAT          0x1000   //draw in combat too

//creature area flags
#define AF_CRE_NOT_LOADED 1
#define AF_NAME_OVERRIDE  8

//getline flags
#define GL_NORMAL         0
#define GL_PASS           1
#define GL_REBOUND        2

//sparkle types
#define SPARKLE_PUFF      1
#define SPARKLE_EXPLOSION 2  //not in the original engine
#define SPARKLE_SHOWER    3

//in areas 10 is a magic number for resref counts
#define MAX_RESCOUNT 10

struct SongHeaderType {
	ieDword SongList[MAX_RESCOUNT];
};

struct RestHeaderType {
	ieDword Strref[MAX_RESCOUNT];
	ieResRef CreResRef[MAX_RESCOUNT];
	ieWord CreatureNum;
	ieWord DayChance;
	ieWord NightChance;
};

struct Entrance {
	ieVariable Name;
	Point Pos;
	ieWord Face;
};

class MapNote {
public:
	ieStrRef strref;
	Point Pos;
	ieWord color;
	char *text;
	MapNote() { text=NULL; }
	~MapNote() { if(text) free(text); }
};

class Spawn {
public:
	ieVariable Name;
	Point Pos;
	ieResRef *Creatures;
	unsigned int Count;
	ieWord Difficulty;
	ieWord Frequency;
	ieWord Method;
	ieByte unknown7c[8];
	ieWord Maximum;
	ieWord Enabled;
	ieDword appearance;
	ieWord DayChance;
	ieWord NightChance;
	Spawn() { Creatures=NULL;  }
	~Spawn() { if(Creatures) free(Creatures); }
	unsigned int GetCreatureCount() { return Count; }
};

class SpawnGroup {
public:
	ieResRef *ResRefs;
	unsigned int Count;
	unsigned int Level;

	SpawnGroup(unsigned int size) {
		ResRefs = (ieResRef *) calloc(size, sizeof(ieResRef) );
		Count = size;
	}
	~SpawnGroup() {
		if (ResRefs) {
			free(ResRefs);
		}
	}
};

class GEM_EXPORT AreaAnimation {
public:
	Animation **animation;
	int animcount;
	//dwords, or stuff combining to a dword
	Point Pos;
	ieDword appearance;
	ieDword Flags;
	//these are on one dword
	ieWord sequence;
	ieWord frame;
	//these are on one dword
	ieWord transparency;
	ieWordSigned height;
	//these are on one dword
	ieWord unknown3c;
	ieByte skipcycle;
	ieByte startchance;
	ieDword unknown48;
	//string values, not in any particular order
	ieVariable Name;
	ieResRef BAM; //not only for saving back (StaticSequence depends on this)
	ieResRef PaletteRef;
	Palette* palette;
	SpriteCover** covers;
	AreaAnimation();
	~AreaAnimation();
	void InitAnimation();
	void SetPalette(ieResRef PaletteRef);
	void BlendAnimation();
	bool Schedule(ieDword gametime);
	void Draw(Region &screen, Map *area);
private:
	Animation *GetAnimationPiece(AnimationFactory *af, int animCycle);
};

enum AnimationObjectType {AOT_AREA, AOT_SCRIPTED, AOT_ACTOR, AOT_SPARK, AOT_PROJECTILE};

//i believe we need only the active actors/visible inactive actors queues
#define QUEUE_COUNT 2

//priorities when handling actors, we really ignore the third one
#define PR_SCRIPT  0
#define PR_DISPLAY 1
#define PR_IGNORE  2

typedef std::list<AreaAnimation*>::iterator aniIterator;
typedef std::list<ScriptedAnimation*>::iterator scaIterator;
typedef std::list<Projectile*>::iterator proIterator;
typedef std::list<Particles*>::iterator spaIterator;

class GEM_EXPORT Map : public Scriptable {
public:
	TileMap* TMap;
	ImageMgr* LightMap;
	ImageMgr* SearchMap;
	ImageMgr* HeightMap;
	ImageMgr* SmallMap;
	IniSpawn *INISpawn;
	ieDword AreaFlags;
	ieWord AreaType;
	ieWord Rain, Snow, Fog, Lightning;
	ieByte* ExploredBitmap;
	ieByte* VisibleBitmap;
	int version;
	ieResRef WEDResRef;
	ieWord localActorCounter;
	bool MasterArea;
	//this is set by the importer (not stored in the file)
	bool DayNight;
	//movies for day/night (only in ToB)
	ieResRef Dream[2];
private:
	ieStrRef trackString;
	int trackFlag;
	ieWord trackDiff;
	unsigned short* MapSet;
	std::queue< unsigned int> InternalStack;
	unsigned int Width, Height;
	std::list< AreaAnimation*> animations;
	std::vector< Actor*> actors;
	Wall_Polygon **Walls;
	unsigned int WallCount;
	std::list< ScriptedAnimation*> vvcCells;
	std::list< Projectile*> projectiles;
	std::list< Particles*> particles;
	std::vector< Entrance*> entrances;
	std::vector< Ambient*> ambients;
	std::vector< MapNote*> mapnotes;
	std::vector< Spawn*> spawns;
	Actor** queue[QUEUE_COUNT];
	int Qcount[QUEUE_COUNT];
	unsigned int lastActorCount[QUEUE_COUNT];
public:
	Map(void);
	~Map(void);
	static void ReleaseMemory();

	/** prints useful information on console */
	void DebugDump();
	TileMap *GetTileMap() { return TMap; }
	/* gets the signal of daylight changes */
	bool ChangeMap(bool day_or_night);
	/* low level function to perform the daylight changes */
	void ChangeTileMap(ImageMgr* lm, ImageMgr* sm);
	/* sets all the auxiliary maps and the tileset */
	void AddTileMap(TileMap* tm, ImageMgr* lm, ImageMgr* sr, ImageMgr* sm, ImageMgr* hm);
	void UpdateScripts();
	bool DoStepForActor(Actor *actor, int speed, ieDword time);
	void UpdateEffects();
	/* removes empty heaps and returns total itemcount */
	int ConsolidateContainers();
	/* transfers all piles (loose items) to another map */
	void CopyGroundPiles(Map *othermap, Point &Pos);
	/* draws stationary vvc graphics */
	//void DrawVideocells(Region screen);
	void DrawHighlightables(Region screen);
	void DrawMap(Region screen);
	void PlayAreaSong(int SongType, bool restart = true);
	void AddAnimation(AreaAnimation* anim);
	aniIterator GetFirstAnimation() { return animations.begin(); }
	AreaAnimation* GetNextAnimation(aniIterator &iter)
	{
		if (iter == animations.end()) {
			return NULL;
		}
		return *iter++;
	}
	AreaAnimation* GetAnimation(const char* Name);
	size_t GetAnimationCount() const { return animations.size(); }

	unsigned int GetWallCount() { return WallCount; }
	Wall_Polygon *GetWallGroup(int i) { return Walls[i]; }
	void SetWallGroups(unsigned int count, Wall_Polygon **walls)
	{
		WallCount = count;
		Walls = walls;
	}
	SpriteCover* BuildSpriteCover(int x, int y, int xpos, int ypos,
		unsigned int width, unsigned int height, int flag);
	void ActivateWallgroups(unsigned int baseindex, unsigned int count, int flg);
	void Shout(Actor* actor, int shoutID, unsigned int radius);
	void ActorSpottedByPlayer(Actor *actor);
	void AddActor(Actor* actor);
	//returns true if an enemy is near P (used in resting/saving)
	bool AnyEnemyNearPoint(Point &p);
	bool GetBlocked(unsigned int x, unsigned int y, unsigned int size);
	int GetBlocked(unsigned int x, unsigned int y);
	int GetBlocked(Point &p);
	Actor* GetActorByGlobalID(ieDword objectID);
	Actor* GetActor(Point &p, int flags);
	Actor* GetActorInRadius(Point &p, int flags, unsigned int radius);
	Actor **GetAllActorsInRadius(Point &p, int flags, unsigned int radius);
	Actor* GetActor(const char* Name, int flags);
	Actor* GetActor(int i, bool any);
	Actor* GetActorByDialog(const char* resref);
	Actor* GetActorByResource(const char* resref);
	bool HasActor(Actor *actor);
	void RemoveActor(Actor* actor);
	//returns actors in rect (onlyparty could be more sophisticated)
	int GetActorInRect(Actor**& actors, Region& rgn, bool onlyparty);
	int GetActorCount(bool any) const;
	//fix actors position if required
	void JumpActors(bool jump);
	//if items == true, remove noncritical items from ground piles too
	void PurgeArea(bool items);

	SongHeaderType SongHeader;
	RestHeaderType RestHeader;

	//count of all projectiles that are saved
	size_t GetProjectileCount(proIterator &iter);
	//get the next projectile
	Projectile *GetNextProjectile(proIterator &iter);
	//count of unexploded projectiles that are saved
	ieDword GetTrapCount(proIterator &iter);
	//get the next saved projectile
	Projectile* GetNextTrap(proIterator &iter);
	//add a projectile to the area
	void AddProjectile(Projectile* pro, Point &source, Point &dest);
	void AddProjectile(Projectile* pro, Point &source, ieWord actorID);

	//returns the duration of a VVC cell set in the area (point may be set to empty)
	ieDword HasVVCCell(const ieResRef resource, Point &p);
	void AddVVCell(ScriptedAnimation* vvc);
	bool CanFree();
	int GetCursor( Point &p);
	//adds a sparkle puff of colour to a point in the area
	//FragAnimID is an optional avatar animation ID (see avatars.2da) for
	//fragment animation
	void Sparkle(ieDword color, ieDword type, Point &pos, unsigned int FragAnimID = 0);
	//removes or fades the sparkle puff at a point
	void FadeSparkle(Point &pos, bool forced);

	//entrances
	void AddEntrance(char* Name, int XPos, int YPos, short Face);
	Entrance* GetEntrance(const char* Name);
	Entrance* GetEntrance(int i) { return entrances[i]; }
	int GetEntranceCount() const { return (int) entrances.size(); }

	//containers
	/* this function returns/creates a pile container at position */
	Container* AddContainer(const char* Name, unsigned short Type,
		Gem_Polygon* outline);
	Container *GetPile(Point &position);
	void AddItemToLocation(Point &position, CREItem *item);

	int GetWidth() const { return Width; }
	int GetHeight() const { return Height; }
	int GetExploredMapSize() const;
	/*fills the explored bitmap with setreset */
	void Explore(int setreset);
	/*fills the visible bitmap with setreset */
	void SetMapVisibility(int setreset = 0);
	/* set one fog tile as visible. x, y are tile coordinates */
	void ExploreTile(Point &Tile);
	/* explore map from given point in map coordinates */
	void ExploreMapChunk(Point &Pos, int range, int los);
	/* block or unblock searchmap with value */
	void BlockSearchMap(Point &Pos, unsigned int size, unsigned int value);
	void ClearSearchMapFor(Movable *actor);
	/* update VisibleBitmap by resolving vision of all explore actors */
	void UpdateFog();
	//PathFinder
	/* Finds the nearest passable point */
	void AdjustPosition(Point &goal, unsigned int radius=0);
	/* Finds the path which leads the farthest from d */
	PathNode* RunAway(Point &s, Point &d, unsigned int size, unsigned int PathLen, int flags);
	/* Returns true if there is no path to d */
	bool TargetUnreachable(Point &s, Point &d, unsigned int size);
	/* returns true if there is enemy visible */
	bool AnyPCSeesEnemy();
	/* Finds straight path from s, length l and orientation o, f=1 passes wall, f=2 rebounds from wall*/
	PathNode* GetLine(Point &start, Point &dest, int flags);
	PathNode* GetLine(Point &start, int Steps, int Orientation, int flags);
	PathNode* GetLine(Point &start, Point &dest, int speed, int Orientation, int flags);
	/* Finds the path which leads to near d */
	PathNode* FindPathNear(const Point &s, const Point &d, unsigned int size, unsigned int MinDistance = 0, bool sight = true);
	/* Finds the path which leads to d */
	PathNode* FindPath(const Point &s, const Point &d, unsigned int size, int MinDistance = 0);
	/* returns false if point isn't visible on visibility/explored map */
	bool IsVisible(const Point &s, int explored);
	/* returns false if point d cannot be seen from point d due to searchmap */
	bool IsVisible(const Point &s, const Point &d);
	/* returns edge direction of map boundary, only worldmap regions */
	int WhichEdge(Point &s);

	//ambients
	void AddAmbient(Ambient *ambient) { ambients.push_back(ambient); }
	void SetupAmbients();
	Ambient *GetAmbient(int i) { return ambients[i]; }
	unsigned int GetAmbientCount() { return (unsigned int) ambients.size(); }

	//mapnotes
	void AddMapNote(Point &point, int color, char *text, ieStrRef strref);
	void RemoveMapNote(Point &point);
	MapNote *GetMapNote(int i) { return mapnotes[i]; }
	MapNote *GetMapNote(Point &point);
	unsigned int GetMapNoteCount() { return (unsigned int) mapnotes.size(); }
	//restheader
	/* May spawn creature(s), returns true in case of an interrupted rest */
	bool Rest(Point &pos, int hours, int day);
	/* Spawns creature(s) in radius of position */
	void SpawnCreature(Point &pos, const char *CreName, int radius = 0);

	//spawns
	void LoadIniSpawn();
	Spawn *AddSpawn(char* Name, int XPos, int YPos, ieResRef *creatures, unsigned int count);
	Spawn *GetSpawn(int i) { return spawns[i]; }
	//returns spawn by name
	Spawn *GetSpawn(const char *Name);
	//returns spawn inside circle, checks for schedule and other
	//conditions as well
	Spawn *GetSpawnRadius(Point &point, unsigned int radius);
	unsigned int GetSpawnCount() { return (unsigned int) spawns.size(); }
	void TriggerSpawn(Spawn *spawn);
	//move some or all players to a new area
	void MoveToNewArea(const char *area, const char *entrance, unsigned int direction, int EveryOne, Actor *actor);
	bool HasWeather();
	int GetWeather();
	void ClearTrap(Actor *actor, ieDword InTrap);
	void SetTrackString(ieStrRef strref, int flg, int difficulty);
	//returns true if tracking failed
	bool DisplayTrackString(Actor *actor);
private:
	AreaAnimation *GetNextAreaAnimation(aniIterator &iter, ieDword gametime);
	Particles *GetNextSpark(spaIterator &iter);
	ScriptedAnimation *GetNextScriptedAnimation(scaIterator &iter);
	Actor *GetNextActor(int &q, int &index);
	void DrawSearchMap(Region &screen);
	void GenerateQueues();
	void SortQueues();
	//Actor* GetRoot(int priority, int &index);
	void DeleteActor(int i);
	void Leveldown(unsigned int px, unsigned int py, unsigned int& level,
		Point &p, unsigned int& diff);
	void SetupNode(unsigned int x, unsigned int y, unsigned int size, unsigned int Cost);
	//actor uses travel region
	void UseExit(Actor *pc, InfoPoint *ip);
	//separated position adjustment, so their order could be randomised */
	bool AdjustPositionX(Point &goal, unsigned int radius);
	bool AdjustPositionY(Point &goal, unsigned int radius);
	void DrawPortal(InfoPoint *ip, int enable);
};

#endif
