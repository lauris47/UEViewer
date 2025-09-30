#include "Core.h"
#include "UnCore.h"

#include "UnObject.h"
#include "UnrealMaterial/UnMaterial.h"
#include "UnrealMaterial/UnMaterial3.h"
#include "UnrealMaterial/UnMaterialExpression.h"
#include "unrealPackage/UnPackage.h"


#include "Exporters.h"


//Hack. get texture type by addition in texture name
FString GetTextureType(const UUnrealMaterial* Tex)
{

	struct TextureSuffix {
		const char* suffix;
		const char* type;
	};

	TextureSuffix suffixMap[] =
	{
		{ "_D", "Diffuse" },
		//{ "_pack", "Diffuse"}, // ... In custom materials, but used common
		{ "_N", "Normal" },
		{ "_MASK", "Mask" },
		{ "_Cube", "Cubemap" },
		{ "_crl", "Detail"}, // ???
	};

	const char* name = Tex->Name;
	int nameLen = strlen(name);


	for (int i = 0; i < ARRAY_COUNT(suffixMap); i++)
	{
		const char* suffix = suffixMap[i].suffix;
		int suffixLen = strlen(suffix);

		if (nameLen >= suffixLen && stricmp(name + nameLen - suffixLen, suffix) == 0)
		{
			appPrintf("Type: %s for texture %s\n", suffixMap[i].type, name);
			return FString(suffixMap[i].type);
		}
	}

	return "";
}

void ExportMaterial(const UUnrealMaterial* Mat)
{
	guard(ExportMaterial);

#if RENDERING				// requires UUnrealMaterial::GetParams()

	if (!Mat) return;

	if (Mat->IsTextureCube())
	{
		ExportCubemap(Mat);
		return;
	}

	if (Mat->IsTexture())
	{
		ExportTexture(Mat);
		return;
	}

	//todo: handle Mat->IsTexture(), Mat->IsTextureCube() to select exporter code
	//todo: remove separate texture handling from Main.cpp exporter registraction


	TArray<UUnrealMaterial*> AllTextures;
	CMaterialParams Params;

	Mat->AppendReferencedTextures(AllTextures, false);

	FArchive* Ar = CreateExportArchive(Mat, EFileArchiveOptions::TextFile, "%s.mat", Mat->Name);
	if (!Ar) return;

	TArray<UObject*> ToExport;


	if (Mat->IsA("Material3"))
	{
		const UMaterial3* Mat3 = static_cast<const UMaterial3*>(Mat);
		for (int i = 0; i < Mat3->ReferencedTextures.Num(); i++)
		{
			UTexture3* Tex = Mat3->ReferencedTextures[i];
			if (Tex)
			{
				appPrintf("Exporting raw texture [%d]: %s\n", i, Tex->Name);
				ToExport.AddUnique(Tex);
			}

		}

		/*
		UMaterialExpressionTextureSampleParameter2D* TexExpr = (UMaterialExpressionTextureSampleParameter2D*)Expr;
		if (TexExpr)
		{
			//appPrintf("BINGO! %s", TexExpr->ParameterName);
		}
		*/
		/*
	}


		if (TexExpr->Texture)
		{
			Ar->Printf("TextureParam2D=%s\n", TexExpr->Texture->Name);
			ToExport.AddUnique(TexExpr->Texture);
		}
		if (TexExpr->ParameterName)
		{
			Ar->Printf("ParamName=%s\n", TexExpr->ParameterName);
		}
		-*
	}


	/*
	else if (!stricmp(ClassName, "MaterialExpressionScalarParameter"))
	{
		auto* ScalarExpr = static_cast<UMaterialExpressionScalarParameter*>(Expr);
		Ar->Printf("ScalarParam=%s Value=%.3f\n", ScalarExpr->ParameterName, ScalarExpr->DefaultValue);
	}
	else if (!stricmp(ClassName, "MaterialExpressionVectorParameter"))
	{
		auto* VectorExpr = static_cast<UMaterialExpressionVectorParameter*>(Expr);
		Ar->Printf("VectorParam=%s Value=(%.3f, %.3f, %.3f, %.3f)\n",
			VectorExpr->ParameterName,
			VectorExpr->DefaultValue.R,
			VectorExpr->DefaultValue.G,
			VectorExpr->DefaultValue.B,
			VectorExpr->DefaultValue.A);
	}
	else if (!stricmp(ClassName, "MaterialExpressionStaticSwitchParameter"))
	{
		auto* SwitchExpr = static_cast<UMaterialExpressionStaticSwitchParameter*>(Expr);
		Ar->Printf("StaticSwitch=%s Default=%s\n",
			SwitchExpr->ParameterName,
			SwitchExpr->DefaultValue ? "True" : "False");
	}
	else
	{
		Ar->Printf("Expression=%s (%s)\n", Expr->Name, ClassName);
	}
	*/



	}

#define PROC(Arg)	\
	if (Params.Arg) \
	{				\
		Ar->Printf(#Arg"=%s\n", Params.Arg->Name); \
		ToExport.AddUnique(Params.Arg); \
	}

	PROC(Diffuse);
	PROC(Normal);
	PROC(Specular);
	PROC(SpecPower);
	PROC(Opacity);
	PROC(Emissive);
	PROC(Cube);
	PROC(Mask);

	//Testing these:
	PROC(Detail);
	PROC(AO);
	PROC(GlowMap);
	PROC(PaintMask);
	PROC(TeamColor);
	PROC(ColorLookup);
	PROC(DecalTexture);
	PROC(TilingPattern);
	PROC(HexMask);
	PROC(Environment);
	PROC(Reflection);
	PROC(Overlay);
	PROC(Noise);
	PROC(Roughness);
	PROC(Metallic);

	// Dump material properties to a separate file
	FArchive* PropAr = CreateExportArchive(Mat, EFileArchiveOptions::TextFile, "%s.props.txt", Mat->Name);
	if (PropAr)
	{
		Mat->GetTypeinfo()->SaveProps(Mat, *PropAr);
		delete PropAr;
	}

	if (Mat->IsA("Material3"))
	{
		const UMaterial3* Material = static_cast<const UMaterial3*>(Mat);
		for (int i = 0; i < Material->Expressions.Num(); i++)
		{
			if (!Material->Expressions[i]) continue;

			// UMaterialExpressionTextureSampleParameter2D is inherited from UMaterialExpressionTextureSampleParameter, so no need to check it separately
			// TODO MaterialExpressionTextureSampleParameterCube
			auto TexSampleParameter = static_cast<const UMaterialExpressionTextureSampleParameter*>(Material->Expressions[i]);
			if (TexSampleParameter)
			{
				if (TexSampleParameter->Texture)
				{
					UTexture3* Tex = TexSampleParameter->Texture;

					if (!Tex) continue;

					const char* paramName = TexSampleParameter->ParameterName;
					const char* texName = Tex->Name;

					if (paramName != "None")
					{
						appPrintf("Found UMaterialExpressionTextureSampleParameter match: %s = %s \n", paramName, texName);
						Ar->Printf("%s=%s\n", paramName, texName);
					}
					ToExport.Add(Tex);
					Tex = nullptr;
				}
				continue;
			}
		}
	}
	

	// HACK - Export other textures
	int numOtherTextures = 0;
	for (int i = 0; i < AllTextures.Num(); i++)
	{
		//Export with texture type name
		UUnrealMaterial* Tex = AllTextures[i];
		if (!Tex) continue;
		//This works but is hacky
		
		FString TextureType = GetTextureType(Tex);
		if (TextureType.Len() > 0)
		{
			Ar->Printf("%s=%s\n", *TextureType, Tex->Name);
		}
		else
		{
			Ar->Printf("Other[%d]=%s\n", numOtherTextures++, Tex->Name);
		}
		
		ToExport.Add(Tex);
	}


	delete Ar; // close .mat file

	// We have done with current object, now let's export referenced objects.

	for (UObject* Obj : ToExport)
	{
		if (Obj != Mat) // UTextureCube::GetParams() adds self to "Cube" field
			ExportObject(Obj);
	}

	// For MaterialInstanceConstant, export its parent too
	if (Mat->IsA("MaterialInstanceConstant"))
	{
		const UMaterialInstanceConstant* Inst = static_cast<const UMaterialInstanceConstant*>(Mat);
		if (Inst->Parent)
		{
			ExportMaterial(Inst->Parent);
		}
	}

#endif // RENDERING

	unguardf("%s'%s'", Mat->GetClassName(), Mat->Name);
}



