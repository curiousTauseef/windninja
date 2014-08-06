/******************************************************************************
 *
 * $Id$
 *
 * Project:  WindNinja
 * Purpose:  OpenFOAM interaction
 * Author:   Kyle Shannon <kyle at pobox dot com>
 *
 ******************************************************************************
 *
 * THIS SOFTWARE WAS DEVELOPED AT THE ROCKY MOUNTAIN RESEARCH STATION (RMRS)
 * MISSOULA FIRE SCIENCES LABORATORY BY EMPLOYEES OF THE FEDERAL GOVERNMENT
 * IN THE COURSE OF THEIR OFFICIAL DUTIES. PURSUANT TO TITLE 17 SECTION 105
 * OF THE UNITED STATES CODE, THIS SOFTWARE IS NOT SUBJECT TO COPYRIGHT
 * PROTECTION AND IS IN THE PUBLIC DOMAIN. RMRS MISSOULA FIRE SCIENCES
 * LABORATORY ASSUMES NO RESPONSIBILITY WHATSOEVER FOR ITS USE BY OTHER
 * PARTIES,  AND MAKES NO GUARANTEES, EXPRESSED OR IMPLIED, ABOUT ITS QUALITY,
 * RELIABILITY, OR ANY OTHER CHARACTERISTIC.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/

#include "ninjafoam.h"

NinjaFoam::NinjaFoam() : ninja()
{
    pszTerrainFile = NULL;
    pszTempPath = NULL;
    pszOgrBase = NULL;
    hGriddedDS = NULL;
    
    boundary_name = "";
    terrainName = "NAME";
    type = "";
    value = "";
    gammavalue = "";
    pvalue = "";
    inletoutletvalue = "";
    template_ = "";
}

/**
 * Copy constructor.
 * @param A Copied value.
 */

NinjaFoam::NinjaFoam(NinjaFoam const& A ) : ninja(A)
{

}

/**
 * Equals operator.
 * @param A Value to set equal to.
 * @return a copy of an object
 */

NinjaFoam& NinjaFoam::operator= (NinjaFoam const& A)
{
    if(&A != this) {
        ninja::operator=(A);
    }
    return *this;
}

NinjaFoam::~NinjaFoam()
{
    free( (void*)pszTerrainFile );
    free( (void*)pszTempPath );
    free( (void*)pszOgrBase );
    GDALClose( hGriddedDS );
}

bool NinjaFoam::simulate_wind()
{

    input.Com->ninjaCom(ninjaComClass::ninjaNone, "Reading elevation file...");

    readInputFile();
    set_position();
    set_uniVegetation();

    checkInputs();
    
    ComputeDirection(); //convert wind direction to unit vector notation
    SetInlets();
    SetBcs();

    #ifdef _OPENMP
    input.Com->ninjaCom(ninjaComClass::ninjaNone, "Run number %d started with %d threads.", input.inputsRunNumber, input.numberCPUs);
    #endif  

    /*------------------------------------------*/
    /*  write OpenFOAM files                    */
    /*------------------------------------------*/

    input.Com->ninjaCom(ninjaComClass::ninjaNone, "Writing OpenFOAM files...");

    int status;

    status = GenerateTempDirectory();
    if(status != 0){
        //do something
    }
    
    status = WriteFoamFiles();
    if(status != 0){
        //do something
    }

    /*-------------------------------------------------------------------*/
    /*  convert DEM to STL format and write to constant/triSurface       */
    /*-------------------------------------------------------------------*/

    input.Com->ninjaCom(ninjaComClass::ninjaNone, "Converting DEM to STL format...");

    const char *pszShortName = CPLGetBasename(input.dem.fileName.c_str());
    const char *pszStlPath = CPLSPrintf("%s/constant/triSurface/", pszTempPath);
    const char *pszStlFileName = CPLFormFilename(pszStlPath, pszShortName, ".stl");

    int nBand = 1;
    const char * inFile = input.dem.fileName.c_str();
    const char * outFile = pszStlFileName;

    CPLErr eErr;

    eErr = NinjaElevationToStl(inFile,
                        outFile,
                        nBand,
                        NinjaStlBinary,
                        NULL);

    if(eErr != 0){
        //do something
    }
    
    /*-------------------------------------------------------------------*/
    /*  write output stl and run surfaceCheck on original stl            */
    /*-------------------------------------------------------------------*/
    
    int nRet;
    
    const char *const papszArgvSurfTransform[] = { "surfaceTransformPoints", 
                                      "-translate", 
                                      CPLSPrintf("(0 0 %.0f)", input.outputWindHeight), 
                                      CPLSPrintf("%s/constant/triSurface/%s.stl", pszTempPath, CPLGetBasename(input.dem.fileName.c_str())), 
                                      CPLSPrintf("%s/constant/triSurface/%s_out.stl", pszTempPath, CPLGetBasename(input.dem.fileName.c_str())),
                                      NULL };
    
    VSILFILE *fout = VSIFOpenL(CPLSPrintf("%s/surfaceTransformPoints.log", pszTempPath), "w");
    
    #ifdef _OPENMP
    input.Com->ninjaCom(ninjaComClass::ninjaNone, "Transforming surface points to output wind height...");
    #endif 
    
    nRet = CPLSpawn(papszArgvSurfTransform, NULL, fout, TRUE); //create output surface stl in pszTemppath/constant/triSurface
    if(nRet != 0){
        //do something
    }
    VSIFCloseL(fout);
    
    const char *const papszArgvSurfCheck[] = { "surfaceCheck", 
                                    CPLSPrintf("%s/constant/triSurface/%s.stl", pszTempPath, CPLGetBasename(input.dem.fileName.c_str())), 
                                    NULL };
    fout = VSIFOpenL(CPLSPrintf("%s/log.json", pszTempPath), "w");
    
    #ifdef _OPENMP
    input.Com->ninjaCom(ninjaComClass::ninjaNone, "Checking surface points in orignal terrain file...");
    #endif
    
    nRet = CPLSpawn(papszArgvSurfCheck, NULL, fout, TRUE); //writes log.json used in mesh file writing
    if(nRet != 0){
        //do something
    }
    VSIFCloseL(fout);
    
    
    
    /*-------------------------------------------------------------------*/
    /*  write contstant/polyMesh/blockMeshDict                           */
    /*-------------------------------------------------------------------*/
    
    //reads from log.json created from surfaceCheck
    status = writeBlockMesh();
    if(status != 0){
        //do something
    }
    
    
    /*-------------------------------------------------------------------*/
    /*  write system/snappyHexMeshDict_cast|layer                        */
    /*-------------------------------------------------------------------*/
    
    status = writeSnappyMesh();
    if(status != 0){
        //do something
    }
    
    
    /*-------------------------------------------------------------------*/
    /* execute commands in run.sh                                        */
    /*-------------------------------------------------------------------*/
    
    //system call: renumberMesh, decomposePar, potentialFoam, simpleFoam, reconstructPar
    
    
    /*-------------------------------------------------------------------*/
    /* convert output files                                              */
    /*-------------------------------------------------------------------*/
       

    return true;
}

int NinjaFoam::AddBcBlock(std::string &dataString)
{
    const char *pszPath = CPLSPrintf( "/vsizip/%s", CPLGetConfigOption( "WINDNINJA_DATA", NULL ) );
    const char *pszTemplateFile;
    const char *pszPathToFile;
    const char *pszTemplate;
    
    if(template_ == ""){
        if(gammavalue != ""){
            pszTemplate = CPLStrdup("genericTypeVal.tmp");
        }
        else if(inletoutletvalue != ""){
            pszTemplate = CPLStrdup("genericType.tmp");
        }
        else{
            pszTemplate = CPLStrdup("genericType-kep.tmp");
        }
    }
    else{
        pszTemplate = CPLStrdup(template_.c_str());
    }

    pszPathToFile = CPLSPrintf("ninjafoam.zip/ninjafoam/0/%s", pszTemplate); 
    pszTemplateFile = CPLFormFilename(pszPath, pszPathToFile, "");

    char *data;
    VSILFILE *fin;
    fin = VSIFOpenL( pszTemplateFile, "r" );
    
    vsi_l_offset offset;
    VSIFSeekL(fin, 0, SEEK_END);
    offset = VSIFTellL(fin);

    VSIRewindL(fin);
    data = (char*)CPLMalloc(offset * sizeof(char) + 1);
    VSIFReadL(data, offset, 1, fin); //read in the template file
    data[offset] = '\0';
    
    std::string s(data);
    
    ReplaceKeys(s, "$boundary_name$", boundary_name);
    ReplaceKeys(s, "$type$", type);
    ReplaceKeys(s, "$value$", value);
    ReplaceKeys(s, "$gammavalue$", gammavalue);
    ReplaceKeys(s, "$pvalue$", pvalue);
    ReplaceKeys(s, "$U_freestream$", boost::lexical_cast<std::string>(input.inputSpeed));
    ReplaceKeys(s, "$direction$", CPLSPrintf("(%.0lf %.0lf %.0lf)", direction[0],
                                                              direction[1],
                                                              direction[2]));
    ReplaceKeys(s, "$InputWindHeight$", boost::lexical_cast<std::string>(input.inputWindHeight));
    ReplaceKeys(s, "$z0$", boost::lexical_cast<std::string>(input.surface.Roughness(0,0)));
    ReplaceKeys(s, "$Rd$", boost::lexical_cast<std::string>(input.surface.Rough_d(0,0)));
    ReplaceKeys(s, "$inletoutletvalue$", inletoutletvalue);
    
    dataString.append(s);
    //cout<<"data in new block = \n"<<s<<endl;
    
    CPLFree(data);
    VSIFCloseL(fin);
    
    return NINJA_SUCCESS;
    
}

int NinjaFoam::WriteZeroFiles(VSILFILE *fin, VSILFILE *fout, const char *pszFilename)
{
    int pos;
    char *data;
        
    vsi_l_offset offset;
    VSIFSeekL(fin, 0, SEEK_END);
    offset = VSIFTellL(fin);

    VSIRewindL(fin);
    data = (char*)CPLMalloc(offset * sizeof(char) + 1);
    VSIFReadL(data, offset, 1, fin); //read in full template file
    data[offset] = '\0';
        
    // write to first dictionary value
    std::string dataString;
    std::string s(data);
    pos = s.find("$boundaryField$");
    if(pos != s.npos){
        s.erase(pos);
        dataString.append(s);
    }
         
    // add boundary field values from .tmp files
    if(std::string(pszFilename) == "p"){
        WritePBoundaryField(dataString);
    }
    
    if(std::string(pszFilename) == "U"){
        WriteUBoundaryField(dataString);
    }
      
    if(std::string(pszFilename) == "k"){
        WriteKBoundaryField(dataString);
    }
      
    if(std::string(pszFilename) == "epsilon"){
        WriteEpsilonBoundaryField(dataString);
    }
    
    // writing remaining fields from template file 
    s = data;
    pos = s.find("$boundaryField$");
    int len = std::string("$boundaryField$").length();
    if(pos != s.npos){
        s.erase(0, pos+len);
    }
    
    ReplaceKeys(s, "$terrainName$", terrainName);
    
    if(input.nonEqBc == 0){
        if(std::string(pszFilename) == "epsilon"){
            ReplaceKeys(s, "$wallFunction$", "epsilonWallFunction");
        }
        else if(std::string(pszFilename) == "nut"){
            ReplaceKeys(s, "$wallFunction$", "nutWallFunction");
        }
    }
    else{
        if(std::string(pszFilename) == "epsilon"){
            ReplaceKeys(s, "$wallFunction$", "epsilonNonEquiWallFunction");
        }
        else if(std::string(pszFilename) == "nut"){
            ReplaceKeys(s, "$wallFunction$", "nutNonEquiWallFunction");
        }
    }
        
    dataString.append(s);
        
    const char * d = dataString.c_str();
    int nSize = strlen(d);
        
    VSIFWriteL( d, nSize, 1, fout );
        
    CPLFree(data);
    
    VSIFCloseL( fin ); // reopened for each file in writeFoamFiles()
    VSIFCloseL( fout ); // reopened for each file in writeFoamFiles()
        
    return NINJA_SUCCESS;
}

int NinjaFoam::WriteSystemFiles(VSILFILE *fin, VSILFILE *fout, const char *pszFilename)
{
    int pos;
    int len; 
    char *data;
        
    vsi_l_offset offset;
    VSIFSeekL(fin, 0, SEEK_END);
    offset = VSIFTellL(fin);

    VSIRewindL(fin);
    data = (char*)CPLMalloc(offset * sizeof(char) + 1);
    VSIFReadL(data, offset, 1, fin); //read in full template file
    data[offset] = '\0';
    
    std::string s(data);
    
    if(std::string(pszFilename) == "decomposeParDict"){
        ReplaceKeys(s, "$nProc$", boost::lexical_cast<std::string>(input.numberCPUs));
        const char * d = s.c_str();
        int nSize = strlen(d);
        VSIFWriteL(d, nSize, 1, fout);
    }
    else if(std::string(pszFilename) == "sampleDict"){
        std::string t = std::string(CPLGetBasename(input.dem.fileName.c_str()));
        t += "_out.stl";
        ReplaceKeys(s, "$stlFileName$", t);
        const char * d = s.c_str();
        int nSize = strlen(d);
        VSIFWriteL(d, nSize, 1, fout);
    }
    else if(std::string(pszFilename) == "controlDict"){
        ReplaceKeys(s, "$finaltime$",boost::lexical_cast<std::string>(input.nIterations));
        const char * d = s.c_str();
        int nSize = strlen(d);
        VSIFWriteL(d, nSize, 1, fout);
    }
    else{
        VSIFWriteL(data, offset, 1, fout);
    }
        
    CPLFree(data);
    
    VSIFCloseL(fin); // reopened for each file in writeFoamFiles()
    VSIFCloseL(fout); // reopened for each file in writeFoamFiles()
        
    return NINJA_SUCCESS;
}

int NinjaFoam::WriteConstantFiles(VSILFILE *fin, VSILFILE *fout, const char *pszFilename)
{
    int pos;
    char *data;
        
    vsi_l_offset offset;
    VSIFSeekL(fin, 0, SEEK_END);
    offset = VSIFTellL(fin);

    VSIRewindL(fin);
    data = (char*)CPLMalloc(offset * sizeof(char) + 1);
    VSIFReadL(data, offset, 1, fin); //read in full template file
    data[offset] = '\0';
    
    VSIFWriteL(data, offset, 1, fout);
        
    CPLFree(data);
    
    VSIFCloseL(fin); // reopened for each file in writeFoamFiles()
    VSIFCloseL(fout); // reopened for each file in writeFoamFiles()
        
    return NINJA_SUCCESS;
}

int NinjaFoam::WriteFoamFiles()
{
    const char *pszPath;
    const char *pszArchive;
    char **papszFileList;
    std::string osFullPath;
    const char *pszFilename;
    const char *pszOutput;
    const char *pszInput;
    const char *pszTempFoamPath;

    //write temporary OpenFOAM directories
    pszPath = CPLSPrintf( "/vsizip/%s", CPLGetConfigOption( "WINDNINJA_DATA", NULL ) );
    pszArchive = CPLSPrintf("%s/ninjafoam.zip/ninjafoam", pszPath);
    papszFileList = VSIReadDirRecursive( pszArchive );
    for(int i = 0; i < CSLCount( papszFileList ); i++){
        pszFilename = CPLGetFilename(papszFileList[i]);
        osFullPath = papszFileList[i];
        if(std::string(pszFilename) == ""){
            pszTempFoamPath = CPLFormFilename(pszTempPath, osFullPath.c_str(), "");
            VSIMkdir(pszTempFoamPath, 0777);
        }
    }

    //write temporary OpenFOAM files
    VSILFILE *fin;
    VSILFILE *fout;
    
    for(int i = 0; i < CSLCount( papszFileList ); i++){
        osFullPath = papszFileList[i];
        pszFilename = CPLGetFilename(papszFileList[i]);
        if(std::string(pszFilename) != "" && 
           std::string(CPLGetExtension(pszFilename)) != "tmp" &&
           std::string(pszFilename) != "snappyHexMeshDict_cast" &&
           std::string(pszFilename) != "snappyHexMeshDict_layer"){
            pszPath = CPLSPrintf( "/vsizip/%s", CPLGetConfigOption( "WINDNINJA_DATA", NULL ) );
            pszArchive = CPLSPrintf("%s/ninjafoam.zip/ninjafoam", pszPath);
            pszInput = CPLFormFilename(pszArchive, osFullPath.c_str(), "");
            pszOutput = CPLFormFilename(pszTempPath, osFullPath.c_str(), "");

            fin = VSIFOpenL( pszInput, "r" );
            fout = VSIFOpenL( pszOutput, "w" );
            
            if( osFullPath.find("0") == 0){
                WriteZeroFiles(fin, fout, pszFilename);
            }
            else if( osFullPath.find("system") == 0 ){
                WriteSystemFiles(fin, fout, pszFilename);
            }
            else if( osFullPath.find("constant") == 0 ){
                WriteConstantFiles(fin, fout, pszFilename);
            }
        }
    }

    CSLDestroy( papszFileList );

    return NINJA_SUCCESS;
}

int NinjaFoam::GenerateTempDirectory()
{
    pszTempPath = CPLStrdup( CPLGenerateTempFilename( NULL ) );
    VSIMkdir( pszTempPath, 0777 );
    pszOgrBase = NULL;

    return NINJA_SUCCESS;
}

void NinjaFoam::SetBcs()
{
    bcs.push_back("east_face");
    bcs.push_back("north_face");
    bcs.push_back("south_face");
    bcs.push_back("west_face");
}

void NinjaFoam::SetInlets()
{
    double d = input.inputDirection;
    if(d == 0 || d == 360){
        inlets.push_back("north_face");
    }
    else if(d == 90){
        inlets.push_back("east_face");
    }
    else if(d == 180){
        inlets.push_back("south_face");
    }
    else if(d == 270){
        inlets.push_back("west_face");
    }
    else if(d > 0 && d < 90){
        inlets.push_back("north_face");
        inlets.push_back("east_face");
    }
    else if(d > 90 && d < 180){
        inlets.push_back("east_face");
        inlets.push_back("south_face");
    }
    else if(d > 180 && d < 270){
        inlets.push_back("south_face");
        inlets.push_back("west_face");
    }
    else if(d > 270 && d < 360){
        inlets.push_back("west_face");
        inlets.push_back("north_face");
    }
}

void NinjaFoam::ComputeDirection()
{
    double d, d1, d2, dx, dy; //CW, d1 is first angle, d2 is second angle
    
    d = input.inputDirection - 180; //convert wind direction from --> wind direction to
    if(d < 0){
        d += 360;
    }
    
    if(d > 0 && d < 90){ //quadrant 1
        d1 = d;
        d2 = 90 - d;
        dx = sin(d1 * PI/180);
        dy = sin(d2 * PI/180);
    }
    else if(d > 90 && d < 180){ //quadrant 2
        d -= 90;
        d1 = d;
        d2 = 90 - d;
        dx = sin(d2 * PI/180);
        dy = -sin(d1 * PI/180);
    }
    else if(d > 180 && d < 270){ //quadrant 3
        d -= 180;
        d1 = d;
        d2 = 90 - d;
        dx = -sin(d1 * PI/180);
        dy = -sin(d2 * PI/180);
    }
    else if(d > 270 && d < 360){ //quadrant 4
        d -= 270;
        d1 = d;
        d2 = 90 - d;
        dx = -sin(d2 * PI/180);
        dy = sin(d1 * PI/180);
    }
    else if(d == 0 || d == 360){
        dx = 0;
        dy = 1;
    }
    else if(d == 90){
        dx = 1;
        dy = 0;
    }
    else if(d == 180){
        dx = 0;
        dy = -1;
    }
    else if(d == 270){
        dx = -1;
        dy = 0;
    }
    
    direction.push_back(dx);
    direction.push_back(dy);
    direction.push_back(0); 
}

int NinjaFoam::WriteOgrVrt( const char *pszSrsWkt )
{
    VSILFILE *fout;
    CPLAssert( pszTempPath );
    CPLAssert( pszOgrBase );
    CPLAssert( pszSrsWkt );
    vsi_l_offset nOffset;
    fout = VSIFOpenL( CPLFormFilename( pszTempPath, pszOgrBase, ".vrt" ),
                      "wb" );
    if( !fout )
    {
        return NINJA_E_FILE_IO;
    }
    const char *pszVrt;
    pszVrt = CPLSPrintf( NINJA_FOAM_OGR_VRT, pszOgrBase, pszOgrBase,
                         pszOgrBase, pszSrsWkt );

    nOffset = VSIFWriteL( pszVrt, CPLStrnlen( pszVrt, 8192 ), 1, fout );
    CPLAssert( nOffset );
    VSIFCloseL( fout );
    return NINJA_SUCCESS;
}

int NinjaFoam::RunGridSampling()
{
    return NINJA_SUCCESS;
}

GDALDatasetH NinjaFoam::GetRasterOutputHandle()
{
    return hGriddedDS;
}

int NinjaFoam::WriteEpsilonBoundaryField(std::string &dataString)
{
    //append BC blocks from template files
    for(int i = 0; i < bcs.size(); i++){
        boundary_name = bcs[i];
        //check if boundary_name is an inlet
        if(std::find(inlets.begin(), inlets.end(), boundary_name) != inlets.end()){
            template_ = "inlet.tmp";
            type = "logProfileDissipationRateInlet";
            value = "";
            gammavalue = "";
            pvalue = "";
            inletoutletvalue = "";
        }
        else{
            template_ = "";
            type = "zeroGradient";
            value = "";
            gammavalue = "";
            pvalue = "";
            inletoutletvalue = "";
        }
        int status;
        //append BC block for current face
        status = AddBcBlock(dataString);
        if(status != 0){
            //do something
        }
    }
        
    return NINJA_SUCCESS;
}

int NinjaFoam::WriteKBoundaryField(std::string &dataString)
{

    //append BC blocks from template files
    for(int i = 0; i < bcs.size(); i++){
        boundary_name = bcs[i];
        //check if boundary_name is an inlet
        if(std::find(inlets.begin(), inlets.end(), boundary_name) != inlets.end()){
            template_ = "inlet.tmp";
            type = "logProfileTurbulentKineticEnergyInlet";
            value = "";
            gammavalue = "";
            pvalue = "";
            inletoutletvalue = "";
        }
        else{
            template_ = "";
            type = "zeroGradient";
            value = "";
            gammavalue = "";
            pvalue = "";
            inletoutletvalue ="";
        }
        int status;
        //append BC block for current face
        status = AddBcBlock(dataString);
        if(status != 0){
            //do something
        }
    }
        
    return NINJA_SUCCESS;
}

int NinjaFoam::WritePBoundaryField(std::string &dataString)
{
    //append BC blocks from template files
    for(int i = 0; i < bcs.size(); i++){
        boundary_name = bcs[i];
        //check if boundary_name is an inlet
        if(std::find(inlets.begin(), inlets.end(), boundary_name) != inlets.end()){
            template_ = "";
            type = "zeroGradient";
            value = "";
            gammavalue = "";
            pvalue = "";
            inletoutletvalue = "";
        }
        else{
            template_ = "";
            type = "totalPressure";
            value = "0";
            gammavalue = "1";
            pvalue = "0";
            inletoutletvalue = "";
        }
        int status;
        //append BC block for current face
        status = AddBcBlock(dataString);
        if(status != 0){
            //do something
        }
    }
        
    return NINJA_SUCCESS;
}

int NinjaFoam::WriteUBoundaryField(std::string &dataString)
{
    //append BC blocks from template files
    for(int i = 0; i < bcs.size(); i++){
        boundary_name = bcs[i];
        //check if boundary_name is an inlet
        if(std::find(inlets.begin(), inlets.end(), boundary_name) != inlets.end()){
            template_ = "inlet.tmp";
            type = "logProfileVelocityInlet";
            value = "";
            gammavalue = "";
            pvalue = "";
            inletoutletvalue = "";
        }
        else{
            template_ = "";
            type = "pressureInletOutletVelocity";
            inletoutletvalue = "(0 0 0)";
            value = "";
            gammavalue = "";
            pvalue = "";
        }
        int status;
        status = AddBcBlock(dataString);
        if(status != 0){
            //do something
        }
    }
        
    return NINJA_SUCCESS;
}

int NinjaFoam::readLogFile(int &ratio)
{
    const char *pszInput;
    
    pszInput = CPLFormFilename(CPLGetCurrentDir(), "log", "json");
    
    VSILFILE *fin;
    fin = VSIFOpenL( pszInput, "r" );
    
    char *data;
    
    vsi_l_offset offset;
    VSIFSeekL(fin, 0, SEEK_END);
    offset = VSIFTellL(fin);

    VSIRewindL(fin);
    data = (char*)CPLMalloc(offset * sizeof(char) + 1);
    VSIFReadL(data, offset, 1, fin);
    data[offset] = '\0';
    
    std::string s(data);
    std:string ss;
    int pos, pos2, pos3, pos4, pos5; 
    int found; 
    pos = s.find("Bounding Box");
    if(pos != s.npos){
        pos2 = s.find("(", pos);
        pos3 = s.find(")", pos2);
        ss = s.substr(pos2+1, pos3-pos2-1); // xmin ymin zmin
        pos4 = s.find("(", pos3);
        pos5 = s.find(")", pos4);
        ss.append(" ");
        ss.append(s.substr(pos4+1, pos5-pos4-1));// xmin ymin zmin xmax ymax zmax
        found = ss.find(" ");
        if(found != ss.npos){
            bbox.push_back(atof(ss.substr(0, found).c_str())); // xmin
            bbox.push_back(atof(ss.substr(found).c_str())); // ymin
        }
        found = ss.find(" ", found+1);
        if(found != ss.npos){
            bbox.push_back(atof(ss.substr(found).c_str())); // zmin
        }
        found = ss.find(" ", found+1);
        if(found != ss.npos){
            bbox.push_back(atof(ss.substr(found).c_str())); // xmax
        }
        found = ss.find(" ", found+1);
        if(found != ss.npos){
            bbox.push_back(atof(ss.substr(found).c_str())); // ymax
        }
        found = ss.find(" ", found+1);
        if(found != ss.npos){
            bbox.push_back(atof(ss.substr(found).c_str()) + 3000); // zmax
            bbox.push_back(atof(ss.substr(found).c_str()) + 1000); // zmid
        }
    }
    else{
        cout<<"Bounding Box not found in log.json!"<<endl;
        return NINJA_E_FILE_IO;
    }
    
    double volume1, volume2;
    double cellCount1, cellCount2;
    double cellVolume1, cellVolume2;
    double side2;
    double firstCellHeight2;
    double expansionRatio;
    
    volume1 = (bbox[3] - bbox[0]) * (bbox[4] - bbox[1]) * (bbox[6] - bbox[2]); // volume near terrain
    volume2 = (bbox[3] - bbox[0]) * (bbox[4] - bbox[1]) * (bbox[5] - bbox[2]); // volume away from terrain
    
    cellCount1 = input.meshCount * 0.5; // cell count in volume 1
    cellCount2 = input.meshCount - cellCount1; // cell count in volume 2
    
    cellVolume1 = volume1/cellCount1; // volume of 1 cell in zone1
    cellVolume2 = volume2/cellCount2; // volume of 1 cell in zone2
    
    side1 = std::pow(cellVolume1, (1.0/3.0)); // length of side of regular hex cell in zone1
    side2 = std::pow(cellVolume2, (1.0/3.0)); // length of side of regular hex cell in zone2
    
    nCells.push_back(int( (bbox[3] - bbox[0]) / side1)); // Nx1
    nCells.push_back(int( (bbox[4] - bbox[1]) / side1)); // Ny1
    nCells.push_back(int( (bbox[6] - bbox[2]) / side1)); // Nz1
    
    nCells.push_back(nCells[0]); // Nx2 = Nx1;
    nCells.push_back(nCells[1]); // Ny2 = Ny1;
    
    expansionRatio = 1.13; // expansion ratio in zone2
    
    firstCellHeight2 = ((bbox[6] - bbox[2]) / nCells[2]) * expansionRatio;
    nCells.push_back(int (log(((bbox[5] - bbox[6]) * (expansionRatio - 1) / firstCellHeight2) + 1) / log(expansionRatio) + 1) ); // Nz2
    ratio = int(std::pow(expansionRatio, (nCells[5] - 1))); // final2oneRatio
    expansionRatio = std::pow(ratio, (1.0 / (nCells[5] - 1)));
    
    CPLFree(data);
    VSIFCloseL(fin);
    
    return NINJA_SUCCESS;
}

int NinjaFoam::writeBlockMesh()
{
    const char *pszInput;
    const char *pszOutput;
    const char *pszPath;
    const char *pszArchive;
    int ratio;
    
    int status;
    status = readLogFile(ratio);
    if(status != 0){
        //do something
    }
    
    pszPath = CPLSPrintf( "/vsizip/%s", CPLGetConfigOption( "WINDNINJA_DATA", NULL ) );
    pszArchive = CPLSPrintf("%s/ninjafoam.zip", pszPath);
    
    pszInput = CPLFormFilename(pszArchive, "ninjafoam/constant/polyMesh/blockMeshDict", "");
    pszOutput = CPLFormFilename(pszTempPath, "constant/polyMesh/blockMeshDict", "");
    
    VSILFILE *fin;
    VSILFILE *fout;
    
    fin = VSIFOpenL( pszInput, "r" );
    fout = VSIFOpenL( pszOutput, "w" );

    char *data;
    
    vsi_l_offset offset;
    VSIFSeekL(fin, 0, SEEK_END);
    offset = VSIFTellL(fin);

    VSIRewindL(fin);
    data = (char*)CPLMalloc(offset * sizeof(char) + 1);
    VSIFReadL(data, offset, 1, fin);
    data[offset] = '\0';
    
    std::string s(data);
    int pos; 
    int len;
    
    bboxField.push_back("$xmin$");
    bboxField.push_back("$ymin$");
    bboxField.push_back("$zmin$");
    bboxField.push_back("$xmax$");
    bboxField.push_back("$ymax$");
    bboxField.push_back("$zmax$");
    bboxField.push_back("$zmid$");
    
    cellField.push_back("$Nx1$");
    cellField.push_back("$Ny1$");
    cellField.push_back("$Nz1$");
    cellField.push_back("$Nx2$");
    cellField.push_back("$Ny2$");
    cellField.push_back("$Nz2$");
    
    for(int i = 0; i<bbox.size(); i++){
        pos = s.find(bboxField[i]);
        len = std::string(bboxField[i]).length();
        while(pos != s.npos){
            std::string t = boost::lexical_cast<std::string>(bbox[i]);
            s.replace(pos, len, t);
            pos = s.find(bboxField[i], pos);
            len = std::string(bboxField[i]).length();
        }
    }
    for(int i = 0; i<nCells.size(); i++){
        pos = s.find(cellField[i]);
        len = std::string(cellField[i]).length();
        while(pos != s.npos){
            std::string t = boost::lexical_cast<std::string>(nCells[i]);
            s.replace(pos, len, t);
            pos = s.find(cellField[i], pos);
            len = std::string(cellField[i]).length();
        }
    }
    
    ReplaceKeys(s, "$Ratio$", boost::lexical_cast<std::string>(ratio));
    
    const char * d = s.c_str();
    int nSize = strlen(d);
    VSIFWriteL(d, nSize, 1, fout);
        
    CPLFree(data);
    VSIFCloseL(fin); 
    VSIFCloseL(fout); 
    
    return NINJA_SUCCESS;
}

int NinjaFoam::writeSnappyMesh()
{
    int lx, ly, lz;
    double expansionRatio;
    double final;
    double first;
    int nLayers;
    
    lx = int((bbox[0] + bbox[3]) * 0.5);
    ly = int((bbox[1] + bbox[4]) * 0.5);
    lz = int((bbox[6]));
    expansionRatio = 1.4;
    final = side1/expansionRatio;
    first = 4.0;
    nLayers = int((log(final/first) / log(expansionRatio)) + 1 + 1);
    
    const char *pszInput;
    const char *pszOutput;
    const char *pszPath;
    const char *pszArchive;
    
    pszPath = CPLSPrintf( "/vsizip/%s", CPLGetConfigOption( "WINDNINJA_DATA", NULL ) );
    pszArchive = CPLSPrintf("%s/ninjafoam.zip", pszPath);
    
    //-----------------------------
    //  write snappyHexMeshDict
    //-----------------------------
    
    pszInput = CPLFormFilename(pszArchive, "ninjafoam/system/snappyHexMeshDict_cast", "");
    pszOutput = CPLFormFilename(pszTempPath, "system/snappyHexMeshDict", "");
    
    VSILFILE *fin;
    VSILFILE *fout;
    
    fin = VSIFOpenL( pszInput, "r" );
    fout = VSIFOpenL( pszOutput, "w" );

    char *data;
    
    vsi_l_offset offset;
    VSIFSeekL(fin, 0, SEEK_END);
    offset = VSIFTellL(fin);

    VSIRewindL(fin);
    data = (char*)CPLMalloc(offset * sizeof(char) + 1);
    VSIFReadL(data, offset, 1, fin);
    data[offset] = '\0';
    
    std::string s(data);
    int pos; 
    int len;
    
    ReplaceKeys(s, "$stlName$", std::string(CPLGetBasename(input.dem.fileName.c_str())));
    ReplaceKeys(s, "$stlRegionName$", "NAME");
    ReplaceKeys(s, "$lx$", boost::lexical_cast<std::string>(lx));
    ReplaceKeys(s, "$ly$", boost::lexical_cast<std::string>(ly));
    ReplaceKeys(s, "$lz$", boost::lexical_cast<std::string>(lz));
    ReplaceKeys(s, "$Nolayers$", boost::lexical_cast<std::string>(nLayers));
    ReplaceKeys(s, "$expansion_ratio$", CPLSPrintf("%.1lf", expansionRatio));
    ReplaceKeys(s, "$final$", boost::lexical_cast<std::string>(final));
    
    const char * d = s.c_str();
    int nSize = strlen(d);
    VSIFWriteL(d, nSize, 1, fout);
        
    CPLFree(data);
    VSIFCloseL(fin); 
    VSIFCloseL(fout); 
    
    //-----------------------------
    //  write snappyHexMeshDict1
    //-----------------------------
    
    pszInput = CPLFormFilename(pszArchive, "ninjafoam/system/snappyHexMeshDict_layer", "");
    pszOutput = CPLFormFilename(pszTempPath, "system/snappyHexMeshDict1", "");
    
    fin = VSIFOpenL( pszInput, "r" );
    fout = VSIFOpenL( pszOutput, "w" );
    
    VSIFSeekL(fin, 0, SEEK_END);
    offset = VSIFTellL(fin);

    VSIRewindL(fin);
    data = (char*)CPLMalloc(offset * sizeof(char) + 1);
    VSIFReadL(data, offset, 1, fin);
    data[offset] = '\0';
    
    s = data;
    
    ReplaceKeys(s, "$stlName$", std::string(CPLGetBasename(input.dem.fileName.c_str())));
    ReplaceKeys(s, "$stlRegionName$", "NAME");
    ReplaceKeys(s, "$lx$", boost::lexical_cast<std::string>(lx));
    ReplaceKeys(s, "$ly$", boost::lexical_cast<std::string>(ly));
    ReplaceKeys(s, "$lz$", boost::lexical_cast<std::string>(lz));
    ReplaceKeys(s, "$Nolayers$", boost::lexical_cast<std::string>(nLayers));
    ReplaceKeys(s, "$expansion_ratio$", boost::lexical_cast<std::string>(expansionRatio));
    ReplaceKeys(s, "$final$", boost::lexical_cast<std::string>(final));
    
    d = s.c_str();
    nSize = strlen(d);
    VSIFWriteL(d, nSize, 1, fout);
        
    CPLFree(data);
    VSIFCloseL(fin); 
    VSIFCloseL(fout); 
    
    return NINJA_SUCCESS;
}

/*
 * Replace key k with value v in string s.  Return 1 if value was replaced, 0
 * if the key was not found
 */
int NinjaFoam::ReplaceKey(std::string &s, std::string k, std::string v)
{
    int i, n;
    i = s.find(k);
    if( i != std::string::npos )
    {
        n = k.length();
        s.replace(i, n, v);
        return TRUE;
    }
    else
        return FALSE;
}

int NinjaFoam::ReplaceKeys(std::string &s, std::string k, std::string v)
{
    int rc = TRUE;
    do
    {
        rc = ReplaceKey(s, k, v);
    } while(rc);
}

