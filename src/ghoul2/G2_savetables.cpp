// The field tables of the Ghoul2 model state that is saved
// (docs/decisions/0015-savegame-format.md). Every member of a structure is named; the
// unit test savetables checks that against the structures themselves.

#include "../server/exe_headers.h"

#include "../game/ghoul2_shared.h"
#include "G2_savetables.h"

// One member to a line.
// clang-format off

SG_TABLE(surfaceInfo_t)
SG_FIELD(offFlags)
SG_FIELD(surface)
SG_FIELD(genBarycentricJ)
SG_FIELD(genBarycentricI)
SG_FIELD(genPolySurfaceIndex)
SG_FIELD(genLod)
SG_TABLE_END(surfaceInfo_t)

SG_TABLE(mdxaBone_t)
SG_FIELD(matrix)
SG_TABLE_END(mdxaBone_t)

SG_TABLE(boneInfo_t)
SG_FIELD(boneNumber)
SG_FIELD(matrix)
SG_FIELD(flags)
SG_FIELD(startFrame)
SG_FIELD(endFrame)
SG_FIELD(startTime)
SG_FIELD(pauseTime)
SG_FIELD(animSpeed)
SG_FIELD(blendFrame)
SG_FIELD(blendLerpFrame)
SG_FIELD(blendTime)
SG_FIELD(blendStart)
SG_FIELD(boneBlendTime)
SG_FIELD(boneBlendStart)
SG_FIELD(newMatrix)
SG_FIELD(lastTimeUpdated)
SG_FIELD(lastContents)
SG_FIELD(lastPosition)
SG_FIELD(velocityEffector)
SG_FIELD(lastAngles)
SG_FIELD(minAngles)
SG_FIELD(maxAngles)
SG_FIELD(currentAngles)
SG_FIELD(anglesOffset)
SG_FIELD(positionOffset)
SG_FIELD(radius)
SG_FIELD(weight)
SG_FIELD(ragIndex)
SG_FIELD(velocityRoot)
SG_FIELD(ragStartTime)
SG_FIELD(firstTime)
SG_FIELD(firstCollisionTime)
SG_FIELD(restTime)
SG_FIELD(RagFlags)
SG_FIELD(DependentRagIndexMask)
SG_FIELD(originalTrueBoneMatrix)
SG_FIELD(parentTrueBoneMatrix)
SG_FIELD(parentOriginalTrueBoneMatrix)
SG_FIELD(originalOrigin)
SG_FIELD(originalAngles)
SG_FIELD(lastShotDir)
SG_OMIT(basepose)
SG_OMIT(baseposeInv)
SG_OMIT(baseposeParent)
SG_OMIT(baseposeInvParent)
SG_FIELD(parentRawBoneIndex)
SG_FIELD(ragOverrideMatrix)
SG_FIELD(extraMatrix)
SG_FIELD(extraVec1)
SG_FIELD(extraFloat1)
SG_FIELD(extraInt1)
SG_FIELD(ikPosition)
SG_FIELD(ikSpeed)
SG_FIELD(epVelocity)
SG_FIELD(epGravFactor)
SG_FIELD(solidCount)
SG_FIELD(physicsSettled)
SG_FIELD(snapped)
SG_FIELD(parentBoneIndex)
SG_FIELD(offsetRotation)
SG_FIELD(overGradSpeed)
SG_FIELD(overGoalSpot)
SG_FIELD(hasOverGoal)
SG_FIELD(animFrameMatrix)
SG_FIELD(hasAnimFrameMatrix)
SG_FIELD(airTime)
SG_TABLE_END(boneInfo_t)

SG_TABLE(boltInfo_t)
SG_FIELD(boneNumber)
SG_FIELD(surfaceNumber)
SG_FIELD(surfaceType)
SG_FIELD(boltUsed)
SG_TABLE_END(boltInfo_t)

// Of a model instance the members from mModelindex to mFlags are saved, as the memory
// between BSAVE_START_FIELD and BSAVE_END_FIELD used to be. The three lists are saved
// element by element by G2_SaveGhoul2Models; the rest is set up again after loading.
SG_TABLE(CGhoul2Info)
SG_OMIT(mSlist)
SG_OMIT(mBltlist)
SG_OMIT(mBlist)
SG_FIELD(mModelindex)
SG_FIELD(animModelIndexOffset)
SG_FIELD(mCustomShader)
SG_FIELD(mCustomSkin)
SG_FIELD(mModelBoltLink)
SG_FIELD(mSurfaceRoot)
SG_FIELD(mLodBias)
SG_FIELD(mNewOrigin)
SG_FIELD(mGoreSetTag)
SG_FIELD(mModel)
SG_FIELD(mFileName)
SG_FIELD(mAnimFrameDefault)
SG_FIELD(mSkelFrameNum)
SG_FIELD(mMeshFrameNum)
SG_FIELD(mFlags)
SG_OMIT(mTransformedVertsArray)
SG_OMIT(mBoneCache)
SG_OMIT(mSkin)
SG_OMIT(mValid)
SG_OMIT(currentModel)
SG_OMIT(currentModelSize)
SG_OMIT(animModel)
SG_OMIT(currentAnimModelSize)
SG_OMIT(aHeader)
SG_TABLE_END(CGhoul2Info)

// clang-format on

std::span<const sg::Table *const> G2_SaveTables() {
	static const sg::Table *const tables[] = {
	    &sg::TableOf<CGhoul2Info>(),
	    &sg::TableOf<surfaceInfo_t>(),
	    &sg::TableOf<boneInfo_t>(),
	    &sg::TableOf<boltInfo_t>(),
	};
	return tables;
}
