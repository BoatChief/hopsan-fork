/*-----------------------------------------------------------------------------

 Copyright 2017 Hopsan Group

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.


 The full license is available in the file LICENSE.
 For details about the 'Hopsan Group' or information about Authors and
 Contributors see the HOPSANGROUP and AUTHORS files that are located in
 the Hopsan source code root directory.

-----------------------------------------------------------------------------*/

#include "GeneratorTypes.h"
#include "GeneratorUtilities.h"

#include "HopsanEssentials.h"
#include "Port.h"

#include <QFile>
#include <QTextStream>
#include <QXmlStreamReader>

bool ComponentLibrary::saveToXML(QString filepath) const
{
    QFile templateFile(":/templates/library_template.xml");
    templateFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&templateFile);
    QString contents = in.readAll();
    templateFile.close();

    contents.replace("<<<libid>>>", mId);
    contents.replace("<<<libname>>>", mName);
    contents.replace("<<<libdebugext>>>", mSharedLibraryDebugExtension);

    // Local help function to generate multiline xml code
    auto writeXmlFileList = [&contents](const QString& pattern, const QString& tagname, const QStringList& files) {
        QString filesXml;
        for (const auto& file : files) {
            filesXml.append(QString("<%1>%2</%1>\n").arg(tagname).arg(file));
        }
        replacePattern(pattern, filesXml, contents);
    };

    writeXmlFileList("<<<sources>>>", "source", mSourceFiles);
    writeXmlFileList("<<<components>>>", "component", mComponentCodeFiles);
    writeXmlFileList("<<<componentxmls>>>", "componentxml", mComponentXMLFiles);
    writeXmlFileList("<<<auxiliaryfiles>>>", "auxiliary", mAuxFiles);

    // Write build flags
    QString compilerFlagsXml, linkerFlagsXml;
    for (const auto& buildFlagSet : mBuildFlags) {
        if (!buildFlagSet.mCompilerFlags.isEmpty()) {
            compilerFlagsXml.append(QString("<cflags os=\"%1\">%2</cflags>\n").arg(buildFlagSet.platformString())
                                                                              .arg(buildFlagSet.mCompilerFlags.join(" ")));
        }
        if (!buildFlagSet.mLinkerFlags.isEmpty()) {
            linkerFlagsXml.append(QString("<lflags os=\"%1\">%2</lflags>\n").arg(buildFlagSet.platformString())
                                                                            .arg(buildFlagSet.mLinkerFlags.join(" ")));
        }
    }
    // Remove final \n
    if (!compilerFlagsXml.isEmpty()) {
        compilerFlagsXml.chop(1);
    }
    if (!linkerFlagsXml.isEmpty()) {
        linkerFlagsXml.chop(1);
    }

    replacePattern("<<<cflags>>>", compilerFlagsXml, contents);
    replacePattern("<<<lflags>>>", linkerFlagsXml, contents);

    QFile outFile(filepath);
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream outStream(&outFile);
        outStream << contents;
        outStream.flush();
        outFile.close();
        return true;
    }
    return false;
}

void ComponentLibrary::clear()
{
    mId.clear();
    mName.clear();
    mSourceFiles.clear();
    mSharedLibraryName.clear();
    mSharedLibraryDebugExtension.clear();
    mComponentCodeFiles.clear();
    mComponentXMLFiles.clear();
    mAuxFiles.clear();
    mBuildFlags.clear();
}

bool ComponentLibrary::loadFromXML(QString filepath)
{
    QFile file(filepath);
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        //mpMessageHandler->addErrorMessage("Cannot open file for reading: "+path);
        return false;
    }
    clear();


    QXmlStreamReader reader(file.readAll());

    bool foundCorrectRootElement=false;
    QString libraryVersion="0.1";
    // Read root element
    while (reader.readNextStartElement()) {
        if(reader.name() == "hopsancomponentlibrary")
        {
            foundCorrectRootElement=true;
            auto attributes = reader.attributes();
            // Read deprecated name attribute if present
            if (attributes.hasAttribute("name")) {
                mName = attributes.value("name").toString();
            }

            libraryVersion = attributes.value("version").toString();
            break;
        }
    }
    if (!foundCorrectRootElement) {
        //mpMessageHandler->addErrorMessage(tr("Not a Hopsan component library! Root tag: %1 != hopsancomponentlibrary").arg(libRoot.tagName()));
        return false;
    }

    // Read contents
    while (reader.readNextStartElement())
    {
        if (reader.name() == "id") {
           mId = reader.readElementText();
        }
        else if (reader.name() == "name") {
            mName = reader.readElementText();
        }
        else if (reader.name() == "lib") {
            mSharedLibraryDebugExtension = reader.attributes().value("debug_ext").toString();
            mSharedLibraryName = reader.readElementText();
        }
        else if (reader.name() == "source") {
            mSourceFiles.append(reader.readElementText());
        }
        else if (reader.name() == "component") {
            mComponentCodeFiles.append(reader.readElementText());
        }
        else if (reader.name() == "componentxml" ||
                 reader.name() == "hopsanobjectappearance" ||
                 reader.name() == "caf") {
            mComponentXMLFiles.append(reader.readElementText());
        }
        else if (reader.name() == "auxiliary") {
            mAuxFiles.append(reader.readElementText());
        }
        else if (reader.name() == "buildflags") {
            //! @todo read build flags
            reader.readElementText();
        }
        else {
            // Discard to proceed
            reader.readElementText();
        }
    }
    file.close();
    return true;
}

PortSpecification::PortSpecification(QString porttype, QString nodetype, QString name, bool notrequired, QString defaultvalue)
{
    this->porttype = porttype;
    this->nodetype = nodetype;
    this->name = name;
    this->notrequired = notrequired;
    this->defaultvalue = defaultvalue;
}


ParameterSpecification::ParameterSpecification(QString name, QString displayName, QString description, QString unit, QString init)
{
    this->name = name;
    this->displayName = displayName;
    this->description = description;
    this->unit = unit;
    this->init = init;
}


VariableSpecification::VariableSpecification(QString name, QString init)
{
    this->name = name;
    this->init = init;
}


ComponentSpecification::ComponentSpecification(QString typeName, QString displayName, QString cqsType)
{
    this->typeName = typeName;
    this->displayName = displayName;
    if(cqsType == "S")
        cqsType = "Signal";
    this->cqsType = cqsType;

    this->auxiliaryFunctions = QStringList();
}

GeneratorNodeInfo::GeneratorNodeInfo(QString nodeType)
{
    //! @todo this will only be able to create the default included nodes (which may be a problem in the future)
    hopsan::HopsanEssentials hopsanCore;
    hopsan::Node *pNode = hopsanCore.createNode(nodeType.toStdString().c_str());
    isValidNode = false;
    if (pNode)
    {
        isValidNode = true;
        niceName = pNode->getNiceName().c_str();
        for(size_t i=0; i<pNode->getDataDescriptions()->size(); ++i)
        {
            const hopsan::NodeDataDescription *pVarDesc = pNode->getDataDescription(i);
            hopsan::NodeDataVariableTypeEnumT varType = pVarDesc->varType;
            // Check if  "Q-type variable"
            if(varType == hopsan::DefaultType || varType == hopsan::FlowType || varType == hopsan::IntensityType)
            {
                qVariables << pVarDesc->shortname.c_str();
                qVariableIds << pVarDesc->id;
                variableLabels << pVarDesc->name.c_str();
                varIdx << pVarDesc->id;
            }
            // Check if "C-type variable"
            else if(varType == hopsan::TLMType)
            {
                cVariables << pVarDesc->shortname.c_str();
                cVariableIds << pVarDesc->id;
                variableLabels << pVarDesc->name.c_str();
                varIdx << pVarDesc->id;
            }

        }
        hopsanCore.removeNode(pNode);
    }
}

void GeneratorNodeInfo::getNodeTypes(QStringList &nodeTypes)
{
    //! @todo this will only be able to list the default included nodes (which may be a problem in the future)
    hopsan::HopsanEssentials hopsanCore;
    std::vector<hopsan::HString> types = hopsanCore.getRegisteredNodeTypes();
    Q_FOREACH(const hopsan::HString &type, types)
    {
        nodeTypes << type.c_str();
    }
}


InterfacePortSpec::InterfacePortSpec(InterfaceTypesEnumT type, QString component, QString port, QStringList path)
{
    this->type = type;
    this->path = path;
    this->component = component;
    this->port = port;

    QStringList inputDataNames;
    QStringList outputDataNames;
    QList<size_t> inputDataIds, outputDataIds;

    switch(type)
    {
    case InterfacePortSpec::Input:
        inputDataNames << "";
        inputDataIds << 0;
        break;
    case InterfacePortSpec::Output:
        outputDataNames << "";
        outputDataIds << 0;
        break;
    case InterfacePortSpec::MechanicQ:
    {
        GeneratorNodeInfo gni("NodeMechanic");
        inputDataNames  << gni.qVariables;
        inputDataIds    << gni.qVariableIds;
        outputDataNames << gni.cVariables;
        outputDataIds   << gni.cVariableIds;
        break;
    }
    case InterfacePortSpec::MechanicC:
    {
        GeneratorNodeInfo gni("NodeMechanic");
        inputDataNames  << gni.cVariables;
        inputDataIds    << gni.cVariableIds;
        outputDataNames << gni.qVariables;
        outputDataIds   << gni.qVariableIds;
        break;
    }
    case InterfacePortSpec::MechanicRotationalQ:
    {
        GeneratorNodeInfo gni("NodeMechanicRotational");
        inputDataNames  << gni.qVariables;
        inputDataIds    << gni.qVariableIds;
        outputDataNames << gni.cVariables;
        outputDataIds   << gni.cVariableIds;
        break;
    }
    case InterfacePortSpec::MechanicRotationalC:
    {
        GeneratorNodeInfo gni("NodeMechanicRotational");
        inputDataNames  << gni.cVariables;
        inputDataIds    << gni.cVariableIds;
        outputDataNames << gni.qVariables;
        outputDataIds   << gni.qVariableIds;
        break;
    }
    case InterfacePortSpec::HydraulicQ:
    {
        GeneratorNodeInfo gni("NodeHydraulic");
        inputDataNames  << gni.qVariables;
        inputDataIds    << gni.qVariableIds;
        outputDataNames << gni.cVariables;
        outputDataIds   << gni.cVariableIds;
        break;
    }
    case InterfacePortSpec::HydraulicC:
    {
        GeneratorNodeInfo gni("NodeHydraulic");
        inputDataNames  << gni.cVariables;
        inputDataIds    << gni.cVariableIds;
        outputDataNames << gni.qVariables;
        outputDataIds   << gni.qVariableIds;
        break;
    }
    case InterfacePortSpec::PneumaticQ:
    {
        GeneratorNodeInfo gni("NodePneumatic");
        inputDataNames  << gni.qVariables;
        inputDataIds    << gni.qVariableIds;
        outputDataNames << gni.cVariables;
        outputDataIds   << gni.cVariableIds;
        break;
    }
    case InterfacePortSpec::PneumaticC:
    {
        GeneratorNodeInfo gni("NodePneumatic");
        inputDataNames  << gni.cVariables;
        inputDataIds    << gni.cVariableIds;
        outputDataNames << gni.qVariables;
        outputDataIds   << gni.qVariableIds;
        break;
    }
    case InterfacePortSpec::ElectricQ:
    {
        GeneratorNodeInfo gni("NodeElectric");
        inputDataNames  << gni.qVariables;
        inputDataIds    << gni.qVariableIds;
        outputDataNames << gni.cVariables;
        outputDataIds   << gni.cVariableIds;
        break;
    }
    case InterfacePortSpec::ElectricC:
    {
        GeneratorNodeInfo gni("NodeElectric");
        inputDataNames  << gni.cVariables;
        inputDataIds    << gni.cVariableIds;
        outputDataNames << gni.qVariables;
        outputDataIds   << gni.qVariableIds;
        break;
    }
    default:
        break;
    }

    foreach(const QString &dataName, inputDataNames)
    {
        vars.append(InterfaceVarSpec(dataName, inputDataIds.takeFirst(), InterfaceVarSpec::Input));
    }
    foreach(const QString &dataName, outputDataNames)
    {
        vars.append(InterfaceVarSpec(dataName, outputDataIds.takeFirst(), InterfaceVarSpec::Output));
    }
}


InterfaceVarSpec::InterfaceVarSpec(QString dataName, int dataId, InterfaceVarSpec::CausalityEnumT causality)
{
    this->dataName = dataName;
    this->dataId = dataId;
    this->causality = causality;
}

void getInterfaces(QList<InterfacePortSpec> &interfaces, hopsan::ComponentSystem *pSystem, QStringList &path)
{
    std::vector<hopsan::HString> names = pSystem->getSubComponentNames();
    for(size_t i=0; i<names.size(); ++i)
    {
        hopsan::Component *pComponent = pSystem->getSubComponent(names[i]);
        hopsan::HString typeName = pComponent->getTypeName();

        if(typeName == "SignalInputInterface")
        {
            interfaces.append(InterfacePortSpec(InterfacePortSpec::Input, names[i].c_str(), "out", path));
        }
        else if(typeName == "SignalOutputInterface")
        {
            interfaces.append(InterfacePortSpec(InterfacePortSpec::Output, names[i].c_str(), "in", path));
        }
        else if(typeName == "MechanicInterfaceQ")
        {
            interfaces.append(InterfacePortSpec(InterfacePortSpec::MechanicQ, names[i].c_str(), "P1", path));
        }
        else if(typeName == "MechanicInterfaceC")
        {
            interfaces.append(InterfacePortSpec(InterfacePortSpec::MechanicC, names[i].c_str(), "P1", path));
        }
        else if(typeName == "MechanicRotationalInterfaceQ")
        {
            interfaces.append(InterfacePortSpec(InterfacePortSpec::MechanicRotationalQ, names[i].c_str(), "P1", path));
        }
        else if(typeName == "MechanicRotationalInterfaceC")
        {
            interfaces.append(InterfacePortSpec(InterfacePortSpec::MechanicRotationalC, names[i].c_str(), "P1", path));
        }
        else if(typeName == "HydraulicInterfaceQ")
        {
            interfaces.append(InterfacePortSpec(InterfacePortSpec::HydraulicQ, names[i].c_str(), "P1", path));
        }
        else if(typeName == "HydraulicInterfaceC")
        {
            interfaces.append(InterfacePortSpec(InterfacePortSpec::HydraulicC, names[i].c_str(), "P1", path));
        }
        else if(typeName == "PneumaticInterfaceQ")
        {
            interfaces.append(InterfacePortSpec(InterfacePortSpec::PneumaticQ, names[i].c_str(), "P1", path));
        }
        else if(typeName == "PneumaticInterfaceC")
        {
            interfaces.append(InterfacePortSpec(InterfacePortSpec::PneumaticC, names[i].c_str(), "P1", path));
        }
        else if(typeName == "ElectricInterfaceQ")
        {
            interfaces.append(InterfacePortSpec(InterfacePortSpec::ElectricQ, names[i].c_str(), "P1", path));
        }
        else if(typeName == "ElectricInterfaceC")
        {
            interfaces.append(InterfacePortSpec(InterfacePortSpec::ElectricC, names[i].c_str(), "P1", path));
        }
        else if(typeName == "Subsystem")
        {
            QStringList path2 = path;
            getInterfaces(interfaces, dynamic_cast<hopsan::ComponentSystem *>(pComponent), path2 << pComponent->getName().c_str());
        }
    }
}



BuildFlags::BuildFlags(const QStringList &cflags, const QStringList &lflags) : mCompilerFlags(cflags), mLinkerFlags(lflags) {}

BuildFlags::BuildFlags(const BuildFlags::Platform platform, const QStringList &cflags, const QStringList &lflags)
    : mCompilerFlags(cflags), mLinkerFlags(lflags), mPlatform(platform) {}

QString BuildFlags::platformString() const {
    switch (mPlatform) {
    case win : return hopsan::os_strings::win ;
    case win32 : return hopsan::os_strings::win32 ;
    case win64 : return hopsan::os_strings::win64 ;
    case Linux : return hopsan::os_strings::Linux ;
    case apple : return hopsan::os_strings::apple ;
    default : return {} ;
    }
}
