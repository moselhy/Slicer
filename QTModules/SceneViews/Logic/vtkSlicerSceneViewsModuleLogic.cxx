// SlicerLogic includes
#include "vtkSlicerSceneViewsModuleLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSceneViewNode.h>
#include <vtkMRMLHierarchyNode.h>

// VTK includes
#include <vtkSmartPointer.h>

// STD includes
#include <string>
#include <iostream>
#include <sstream>

// Convenient macro
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSlicerSceneViewsModuleLogic, "$Revision: 1.0$")
vtkStandardNewMacro(vtkSlicerSceneViewsModuleLogic)

//-----------------------------------------------------------------------------
// vtkSlicerSceneViewsModuleLogic methods
//-----------------------------------------------------------------------------
vtkSlicerSceneViewsModuleLogic::vtkSlicerSceneViewsModuleLogic()
{
  this->m_Widget = 0;
  this->m_LastAddedSceneViewNode = 0;
  this->m_ActiveHierarchy = 0;
}

//-----------------------------------------------------------------------------
vtkSlicerSceneViewsModuleLogic::~vtkSlicerSceneViewsModuleLogic()
{
  if (this->m_Widget)
    {
    this->m_Widget = 0;
    }

  if (this->m_LastAddedSceneViewNode)
    {
    this->m_LastAddedSceneViewNode->Delete();
    this->m_LastAddedSceneViewNode = 0;
    }

  if (this->m_ActiveHierarchy)
    {
    this->m_ActiveHierarchy->Delete();
    this->m_ActiveHierarchy = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
/// Set and observe the GUI widget
//-----------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::SetAndObserveWidget(qSlicerSceneViewsModuleWidget* widget)
{
  if (!widget)
    {
    return;
    }

  this->m_Widget = widget;

}

//---------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::InitializeEventListeners()
{
  vtkDebugMacro("InitializeEventListeners");
  // a good time to add the observed events!
  vtkIntArray *events = vtkIntArray::New();
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkCommand::ModifiedEvent);
  events->InsertNextValue(vtkMRMLScene::SceneClosedEvent);
  events->InsertNextValue(vtkMRMLScene::SceneImportedEvent);
  this->SetAndObserveMRMLSceneEventsInternal(this->GetMRMLScene(), events);
  events->Delete();
}

//-----------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::ProcessMRMLEvents(
  vtkObject* vtkNotUsed(caller), unsigned long event, void * callData)
{
  // on scene closed event need to call:
  //this->m_ActiveHierarchy = 0;
  vtkDebugMacro("ProcessMRMLEvents: Event "<< event);

  vtkMRMLNode* node = reinterpret_cast<vtkMRMLNode*> (callData);

  if (event==vtkMRMLScene::SceneClosedEvent)
    {
    this->OnMRMLSceneClosedEvent();
    return;
    }
  if (event == vtkMRMLScene::SceneImportedEvent)
    {
    this->OnMRMLSceneImportedEvent();
    return;
    }
  
  vtkMRMLSceneViewNode* sceneViewNode = vtkMRMLSceneViewNode::SafeDownCast(node);
  if (!sceneViewNode)
    {
    return;
    }

  switch (event)
    {
    case vtkMRMLScene::NodeAddedEvent:
      this->OnMRMLSceneNodeAddedEvent(sceneViewNode);
      break;
    case vtkCommand::ModifiedEvent:
      this->OnMRMLSceneViewNodeModifiedEvent(sceneViewNode);
      break;

    }
}

//-----------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::OnMRMLSceneNodeAddedEvent(vtkMRMLNode* node)
{
  vtkDebugMacro("OnMRMLSceneNodeAddedEvent");

  // don't do anything if the scene is still updating
  if (this->GetMRMLScene() &&
      this->GetMRMLScene()->GetIsUpdating())
    {
    vtkDebugMacro("OnMRMLSceneNodeAddedEvent: updating, returning");
    return;
    }
  
  vtkMRMLSceneViewNode * sceneViewNode = vtkMRMLSceneViewNode::SafeDownCast(node);
  if (!sceneViewNode)
    {
    return;
    }
  
  int retval = this->AddHierarchyNodeForNode(sceneViewNode);
  vtkMRMLHierarchyNode* hierarchyNode = NULL;
  if (!retval)
    {
    vtkErrorMacro("OnMRMLSceneNodeAddedEvent: error adding a hierarchy node for scene view node");
    return;
    }
  hierarchyNode =  vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(sceneViewNode->GetScene(), sceneViewNode->GetID());
  if (!hierarchyNode)
    {
    vtkErrorMacro("OnMRMLSceneNodeAddedEvent: No hierarchyNode found.")
    return;
    }
  hierarchyNode->SetAssociatedNodeID(sceneViewNode->GetID());
  sceneViewNode->Modified();

  // we pass the hierarchy node along - it includes the pointer to the actual sceneViewNode
  this->AddNodeCompleted(hierarchyNode);
}

//-----------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::OnMRMLSceneImportedEvent()
{
  vtkDebugMacro("OnMRMLSceneImportedEvent");
  // update from the updated scene
  if (this->m_Widget)
    {
    this->m_Widget->refreshTree();
    }
}

//-----------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::OnMRMLSceneViewNodeModifiedEvent(vtkMRMLNode* node)
{
  vtkDebugMacro("OnMRMLSceneViewNodeModifiedEvent " << node->GetID());

  if (this->GetMRMLScene() &&
      this->GetMRMLScene()->GetIsUpdating())
    {
    vtkDebugMacro("OnMRMLSceneViewNodeModifiedEvent: updating, returning");
    return;
    }
  
  vtkMRMLSceneViewNode * sceneViewNode = vtkMRMLSceneViewNode::SafeDownCast(node);
  if (!sceneViewNode)
    {
    return;
    }

  // refresh the hierarchy tree
  if (this->m_Widget)
    {
    this->m_Widget->refreshTree();
    }
}

//-----------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::OnMRMLSceneClosedEvent()
{
  if (this->m_LastAddedSceneViewNode)
    {
    this->m_LastAddedSceneViewNode = 0;
    }

  if (this->m_ActiveHierarchy)
    {
    this->m_ActiveHierarchy = 0;
    }
  if (this->m_Widget)
    {
    this->m_Widget->refreshTree();
    }
}


//-----------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::RegisterNodes()
{

  if (!this->GetMRMLScene())
    {
    std::cerr << "RegisterNodes: no scene on which to register nodes" << std::endl;
    return;
    }


  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::New();
  this->GetMRMLScene()->RegisterNodeClass(viewNode);
  viewNode->Delete();

  vtkMRMLSceneViewStorageNode *storageNode = vtkMRMLSceneViewStorageNode::New();
  this->GetMRMLScene()->RegisterNodeClass ( storageNode );
  storageNode->Delete();
  
  // set up the event listeners now
  this->InitializeEventListeners();


}

//---------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::CreateSceneView(const char* name, const char* description, int screenshotType, vtkImageData* screenshot)
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return;
    }

  if (!screenshot)
    {
    vtkErrorMacro("CreateSceneView: No screenshot was set.")
    return;
    }

  vtkStdString nameString = vtkStdString(name);

  vtkMRMLSceneViewNode * newSceneViewNode = vtkMRMLSceneViewNode::New();
  newSceneViewNode->SetScene(this->GetMRMLScene());
  if (strcmp(nameString,""))
    {
    // a name was specified
    newSceneViewNode->SetName(nameString.c_str());
    }
  else
    {
    // if no name is specified, generate a new unique one
    newSceneViewNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("SceneView"));
    }

  vtkStdString descriptionString = vtkStdString(description);

  newSceneViewNode->SetSceneViewDescription(descriptionString);
  newSceneViewNode->SetScreenShotType(screenshotType);

  // make a new vtk image data, as the set macro is taking the pointer
  vtkSmartPointer<vtkImageData> copyScreenShot = vtkSmartPointer<vtkImageData>::New();
  copyScreenShot->DeepCopy(screenshot);
  newSceneViewNode->SetScreenShot(copyScreenShot);
  newSceneViewNode->StoreScene();
  //newSceneViewNode->HideFromEditorsOff();
  
  this->GetMRMLScene()->AddNode(newSceneViewNode);

  // put it in a hierarchy
  if (!this->AddHierarchyNodeForNode(newSceneViewNode))
    {
    vtkErrorMacro("CreateSceneView: Error adding a hierarchy node for new scene view node " << newSceneViewNode->GetID());
    }

  newSceneViewNode->Delete();
}

//---------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::ModifySceneView(vtkStdString id, const char* name, const char* description, int screenshotType, vtkImageData* screenshot)
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return;
    }

  if (!screenshot)
    {
    vtkErrorMacro("ModifySceneView: No screenshot was set.")
    return;
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id.c_str()));

  if (!viewNode)
    {
    vtkErrorMacro("GetSceneViewName: Could not get sceneView node!")
    return;
    }

  vtkStdString nameString = vtkStdString(name);
  if (strcmp(nameString,""))
    {
    // a name was specified
    viewNode->SetName(nameString.c_str());
    }
  else
    {
    // if no name is specified, generate a new unique one
    viewNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("SceneView"));
    }

  vtkStdString descriptionString = vtkStdString(description);
  viewNode->SetSceneViewDescription(descriptionString);
  viewNode->SetScreenShotType(screenshotType);
  viewNode->SetScreenShot(screenshot);

  // TODO why two events?
  viewNode->Modified();
  viewNode->GetScene()->InvokeEvent(vtkCommand::ModifiedEvent, viewNode);

}

//---------------------------------------------------------------------------
vtkStdString vtkSlicerSceneViewsModuleLogic::GetSceneViewName(const char* id)
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return 0;
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id));

  if (!viewNode)
    {
    vtkErrorMacro("GetSceneViewName: Could not get sceneView node!")
    return 0;
    }

  return vtkStdString(viewNode->GetName());
}

//---------------------------------------------------------------------------
vtkStdString vtkSlicerSceneViewsModuleLogic::GetSceneViewDescription(const char* id)
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return 0;
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id));

  if (!viewNode)
    {
    vtkErrorMacro("GetSceneViewDescription: Could not get sceneView node!")
    return 0;
    }

  return viewNode->GetSceneViewDescription();
}

//---------------------------------------------------------------------------
int vtkSlicerSceneViewsModuleLogic::GetSceneViewScreenshotType(const char* id)
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return -1;
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id));

  if (!viewNode)
    {
    vtkErrorMacro("GetSceneViewScreenshotType: Could not get sceneView node!")
    return -1;
    }

  return viewNode->GetScreenShotType();
}

//---------------------------------------------------------------------------
vtkImageData* vtkSlicerSceneViewsModuleLogic::GetSceneViewScreenshot(const char* id)
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return 0;
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id));

  if (!viewNode)
    {
    vtkErrorMacro("GetSceneViewScreenshot: Could not get sceneView node!")
    return 0;
    }

  return viewNode->GetScreenShot();
}

//---------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::RestoreSceneView(const char* id)
{
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return;
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id));

  if (!viewNode)
    {
    vtkErrorMacro("RestoreSceneView: Could not get sceneView node!")
    return;
    }

  this->GetMRMLScene()->SaveStateForUndo();
  viewNode->RestoreScene();
}

//---------------------------------------------------------------------------
const char* vtkSlicerSceneViewsModuleLogic::MoveSceneViewUp(const char* id)
{
  // reset stringHolder
  this->m_StringHolder = "";

  if (!id)
    {
    return this->m_StringHolder.c_str();
    }

  this->m_StringHolder = id;

  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return this->m_StringHolder.c_str();
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id));

  if (!viewNode)
    {
    vtkErrorMacro("MoveSceneViewUp: Could not get sceneView node! (id = " << id << ")")
    return this->m_StringHolder.c_str();
    }

  // see if it's in a hierarchy
  vtkMRMLHierarchyNode *hNode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(this->GetMRMLScene(), id);
  if (!hNode)
    {
    vtkWarningMacro("MoveSceneViewUp: did not find a hierarchy node for node with id " << id);
    return this->m_StringHolder.c_str();
    }
  // where is it in the parent's list?
  int currentIndex = hNode->GetIndexInParent();
  // now move it up one
  hNode->SetIndexInParent(currentIndex - 1);
  // if it succeeded, trigger a modified event on the node to get the GUI to
  // update
  if (hNode->GetIndexInParent() != currentIndex)
    {
    std::cout << "MoveSceneViewUp: calling mod on scene view node" << std::endl;
    viewNode->Modified();
    }
  // the id should be the same now
  this->m_StringHolder = viewNode->GetID();
  return this->m_StringHolder.c_str();
}

//---------------------------------------------------------------------------
const char* vtkSlicerSceneViewsModuleLogic::MoveSceneViewDown(const char* id)
{
  // reset stringHolder
  this->m_StringHolder = "";

  if (!id)
    {
    return this->m_StringHolder.c_str();
    }

  this->m_StringHolder = id;


  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("No scene set.")
    return this->m_StringHolder.c_str();
    }

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id));

  if (!viewNode)
    {
    vtkErrorMacro("MoveSceneViewDown: Could not get sceneView node!")
    return this->m_StringHolder.c_str();
    }

  // see if it's in a hierarchy
  vtkMRMLHierarchyNode *hNode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(this->GetMRMLScene(), id);
  if (!hNode)
    {
    vtkWarningMacro("MoveSceneViewDown: Temporarily disabled (did not find a hierarchy node for node with id " << id << ")");
    return this->m_StringHolder.c_str();
    }
  // where is it in the parent's list?
  int currentIndex = hNode->GetIndexInParent();
  // now move it down one
  hNode->SetIndexInParent(currentIndex + 1);
  // the id should be the same now
  this->m_StringHolder = viewNode->GetID();
  return this->m_StringHolder.c_str();
}

//---------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::RemoveSceneViewNode(vtkMRMLSceneViewNode *sceneViewNode)
{
  if (!sceneViewNode)
    {
    vtkErrorMacro("RemoveSceneViewNode: No node to remove");
    return;
    }

  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("RemoveSceneViewNode: No MRML Scene found from which to remove the node");
    return;
    }

  // remove the 1-1 IS-A hierarchy node first
  vtkMRMLHierarchyNode *hNode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(this->GetMRMLScene(), sceneViewNode->GetID());
  if (hNode)
    {
    // there is a parent node
    this->GetMRMLScene()->RemoveNode(hNode);
    }

  this->GetMRMLScene()->RemoveNode(sceneViewNode);

}

//---------------------------------------------------------------------------
int vtkSlicerSceneViewsModuleLogic::AddHierarchyNodeForNode(vtkMRMLNode* node)
{
  // check that there isn't already a hierarchy node for this node
  if (node && node->GetScene() && node->GetID())
    {
    vtkMRMLHierarchyNode *hnode = vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(node->GetScene(), node->GetID());
    if (hnode != NULL)
      {
      vtkDebugMacro("AddHierarchyNodeForNode: scene view node " << node->GetID() << " already has a hierarchy node, returning.");
      return true;
      }
    }
  if (!this->m_ActiveHierarchy)
    {
    vtkDebugMacro("AddHierarchyNodeForNode: no active hierarchy...");
    // no active hierarchy node, this means we create the new node directly under the top-level hierarchy node
    vtkMRMLHierarchyNode* toplevelHierarchyNode = 0;
    if (!node)
      {
      // we just add a new toplevel hierarchy node
      toplevelHierarchyNode = this->GetTopLevelHierarchyNode(0);
      }
    else
      {
      // we need to insert the new toplevel hierarchy before the given node
      toplevelHierarchyNode = this->GetTopLevelHierarchyNode(node);
      }

    this->m_ActiveHierarchy = toplevelHierarchyNode;

    if (!toplevelHierarchyNode)
      {
      vtkErrorMacro("AddHierarchyNodeForNode: Toplevel hierarchy node was NULL.")
      return 0;
      }

    }

  // Create a hierarchy node
  vtkMRMLHierarchyNode* hierarchyNode = vtkMRMLHierarchyNode::New();
  if (hierarchyNode == NULL)
    {
    vtkErrorMacro("AddHierarchyNodeForNode: can't create a new hierarhcy node to associate with scene view " << node->GetID());
    return 0;
    }

  hierarchyNode->SetParentNodeID(this->m_ActiveHierarchy->GetID());
  hierarchyNode->SetScene(this->GetMRMLScene());

  if (!node)
    {
    // this is a user created hierarchy!

    // we want to see that!
    hierarchyNode->HideFromEditorsOff();

    hierarchyNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("SceneViewHierarchy"));

    this->GetMRMLScene()->AddNode(hierarchyNode);

    // we want it to be the active hierarchy from now on
    this->m_ActiveHierarchy = hierarchyNode;
    }
  else
    {
    // this is the 1-1 hierarchy node for a given node

    // we do not want to see that!
    hierarchyNode->HideFromEditorsOn();

    hierarchyNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("SceneViewHierarchy"));

    this->GetMRMLScene()->InsertBeforeNode(node,hierarchyNode);
    hierarchyNode->SetAssociatedNodeID(node->GetID());
    vtkDebugMacro("AddHierarchyNodeForNode: added hierarchy node, id = " << (hierarchyNode->GetID() ? hierarchyNode->GetID() : "null") << ", set associated node id on the hierarhcy node of " << (hierarchyNode->GetAssociatedNodeID() ? hierarchyNode->GetAssociatedNodeID() : "null"));
    }
  
  hierarchyNode->Delete();
  return 1;
}

//---------------------------------------------------------------------------
int vtkSlicerSceneViewsModuleLogic::AddHierarchy()
{
  return this->AddHierarchyNodeForNode(0);
}

//---------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkSlicerSceneViewsModuleLogic::GetTopLevelHierarchyNode(vtkMRMLNode* node)
{

  if (this->GetMRMLScene() == NULL)
    {
    return NULL;
    }
  const char *toplevelName = "SceneViewToplevelHierarchyNode";
  //vtkMRMLHierarchyNode* toplevelNode = vtkMRMLHierarchyNode::SafeDownCast(this->GetMRMLScene()->GetNthNodeByClass(0,"vtkMRMLHierarchyNode"));
  vtkCollection *col = this->GetMRMLScene()->GetNodesByClass("vtkMRMLHierarchyNode");
  vtkMRMLHierarchyNode* toplevelNode = NULL;
  unsigned int numNodes = col->GetNumberOfItems();
  if (numNodes != 0)
    {
    //std::cout << "Found " << numNodes << " hierarchy nodes" << std::endl;
    // iterate through the hierarchy nodes to find one with a name starting
    // with the top level name
    for (unsigned int n = 0; n < numNodes; n++)
      {
      vtkMRMLNode *thisNode = vtkMRMLNode::SafeDownCast(col->GetItemAsObject(n));
      //std::cout << "\tthisNode name = '" << thisNode->GetName() << "', toplevelname = '" << toplevelName << "', strlen = " << strlen(toplevelName) << std::endl;
      if (thisNode && thisNode->GetName() &&
          strncmp(thisNode->GetName(), toplevelName, strlen(toplevelName)) == 0)
        {
        toplevelNode = vtkMRMLHierarchyNode::SafeDownCast(col->GetItemAsObject(n));
        //std::cout << "\tfound matching hierarchy node at index " << n << ", named " << toplevelNode->GetName() << std::endl;
        break;
        }
      }
    }
  if (!toplevelNode)
    {
    //std::cout << "GetTopLevelHierarchyNode: no top level node, making new" << std::endl;
    // no hierarchy node is currently in the scene, create a new one
    toplevelNode = vtkMRMLHierarchyNode::New();
    toplevelNode->HideFromEditorsOff();
    toplevelNode->SetName(this->GetMRMLScene()->GetUniqueNameByString(toplevelName));

    if (!node)
      {
      this->GetMRMLScene()->AddNode(toplevelNode);
      }
    else
      {
      this->GetMRMLScene()->InsertBeforeNode(node,toplevelNode);
      }
    }
  col->RemoveAllItems();
  col->Delete();
  // if delete it when created it, get a seg fault on clearing the scene, no
  // leaks like this so far
  return toplevelNode;
}

//---------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::SetActiveHierarchyNode(vtkMRMLHierarchyNode* hierarchyNode)
{
  if (!hierarchyNode)
    {
    // there was no node as input
    // we then use the toplevel hierarchyNode
    vtkMRMLHierarchyNode* toplevelNode = this->GetTopLevelHierarchyNode();

    if (!toplevelNode)
      {
      vtkErrorMacro("SetActiveHierarchyNodeByID: Could not find or create any hierarchy.")
      return;
      }

    this->m_ActiveHierarchy = toplevelNode;

    return;
    }

  this->m_ActiveHierarchy = hierarchyNode;
}

//---------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::SetActiveHierarchyNodeByID(const char* id)
{
  vtkMRMLHierarchyNode* hierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(id));

  this->SetActiveHierarchyNode(hierarchyNode);
}

//---------------------------------------------------------------------------
const char *vtkSlicerSceneViewsModuleLogic::GetActiveHierarchyNodeID()
{
  if (this->m_ActiveHierarchy)
    {
    return this->m_ActiveHierarchy->GetID();
    }
  else
    {
    return NULL;
    }
}

//---------------------------------------------------------------------------
void vtkSlicerSceneViewsModuleLogic::AddNodeCompleted(vtkMRMLHierarchyNode* hierarchyNode)
{

  if (!hierarchyNode)
    {
    return;
    }

  if (!this->m_Widget)
    {
    return;
    }

  vtkMRMLSceneViewNode* sceneViewNode = vtkMRMLSceneViewNode::SafeDownCast(hierarchyNode->GetAssociatedNode());

  if (!sceneViewNode)
    {
    vtkErrorMacro("AddNodeCompleted: Could not get scene view node.")
    return;
    }

  // refresh the hierarchy tree
  if (this->m_Widget)
    {
    this->m_Widget->refreshTree();
    }

  this->m_LastAddedSceneViewNode = sceneViewNode;

}
