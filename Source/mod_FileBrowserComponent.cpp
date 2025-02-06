/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

#include "mod_FileBrowserComponent.h"
#include <juce_gui_basics/detail/juce_WindowingHelpers.h>

namespace juce::mod
{

    FileBrowserComponent::FileBrowserComponent(int flags_,
                                            const File& initialFileOrDirectory,
                                            const FileFilter* fileFilter_,
                                            FilePreviewComponent* previewComp_,
                                            OwnLookAndFeel* myLookAndFeel_)
   : FileFilter ({}),
     fileFilter (fileFilter_),
     flags (flags_),
     previewComp (previewComp_),
     myLookAndFeel(myLookAndFeel_),
     currentPathBox ("path"),
     //fileLabel ("f", TRANS ("file:")),
     thread (SystemStats::getJUCEVersion() + ": FileBrowser"),
     wasProcessActive (true)
{
    // You need to specify one or other of the open/save flags..
    jassert ((flags & (saveMode | openMode)) != 0);
    jassert ((flags & (saveMode | openMode)) != (saveMode | openMode));

    // You need to specify at least one of these flags..
    jassert ((flags & (canSelectFiles | canSelectDirectories)) != 0);

    String filename;

    if (initialFileOrDirectory == File())
    {
        currentRoot = File::getCurrentWorkingDirectory();
    }
    else if (initialFileOrDirectory.isDirectory())
    {
        currentRoot = initialFileOrDirectory;
    }
    else
    {
        chosenFiles.add (initialFileOrDirectory);
        currentRoot = initialFileOrDirectory.getParentDirectory();
        filename = initialFileOrDirectory.getFileName();
    }

    // The thread must be started before the DirectoryContentsList attempts to scan any file
    thread.startThread (Thread::Priority::low);

    fileList.reset (new DirectoryContentsList (this, thread));
    fileList->setDirectory (currentRoot, true, true);

    if ((flags & useTreeView) != 0)
    {
        auto tree = new FileTreeComponent (*fileList);
        fileListComponent.reset (tree);

        if ((flags & canSelectMultipleItems) != 0)
            tree->setMultiSelectEnabled (true);

        addAndMakeVisible (tree);
    }
    else
    {
        auto list = new FileListComponent (*fileList);
        fileListComponent.reset (list);
        list->setOutlineThickness (1);

        if ((flags & canSelectMultipleItems) != 0)
            list->setMultipleSelectionEnabled (true);

        addAndMakeVisible (list);
    }

    fileListComponent->addListener (this);

    addAndMakeVisible (currentPathBox);
    currentPathBox.setEditableText (true);
    resetRecentPaths();
    currentPathBox.onChange = [this] { updateSelectedPath(); };

    {
        if (! isSaveMode())
            selectionChanged();
    };


    if (previewComp != nullptr)
        addAndMakeVisible (previewComp);

    setLookAndFeel(myLookAndFeel);
    //lookAndFeelChanged();

    setRoot (currentRoot);

    startTimer (2000);
}

FileBrowserComponent::~FileBrowserComponent()
{
    fileListComponent.reset();
    fileList.reset();
    thread.stopThread (10000);
}

//==============================================================================
void FileBrowserComponent::addListener (FileBrowserListener* const newListener)
{
    listeners.add (newListener);
}

void FileBrowserComponent::removeListener (FileBrowserListener* const listener)
{
    listeners.remove (listener);
}

//==============================================================================
bool FileBrowserComponent::isSaveMode() const noexcept
{
    return (flags & saveMode) != 0;
}

int FileBrowserComponent::getNumSelectedFiles() const noexcept
{
    if (chosenFiles.isEmpty() && currentFileIsValid())
        return 1;

    return chosenFiles.size();
}

File FileBrowserComponent::getSelectedFile (int index) const noexcept
{
    if ((flags & canSelectDirectories) != 0)
        return currentRoot;

    //if (! filenameBox.isReadOnly())
    //    return currentRoot.getChildFile (filenameBox.getText());

    return chosenFiles[index];
}

bool FileBrowserComponent::currentFileIsValid() const
{
    auto f = getSelectedFile (0);

    if ((flags & canSelectDirectories) == 0 && f.isDirectory())
        return false;

    return isSaveMode() || f.exists();
}

File FileBrowserComponent::getHighlightedFile() const noexcept
{
    return fileListComponent->getSelectedFile (0);
}

void FileBrowserComponent::deselectAllFiles()
{
    fileListComponent->deselectAllFiles();
}

//==============================================================================
bool FileBrowserComponent::isFileSuitable (const File& file) const
{
    return (flags & canSelectFiles) != 0
             && (fileFilter == nullptr || fileFilter->isFileSuitable (file));
}

bool FileBrowserComponent::isDirectorySuitable (const File&) const
{
    return true;
}

bool FileBrowserComponent::isFileOrDirSuitable (const File& f) const
{
    if (f.isDirectory())
        return (flags & canSelectDirectories) != 0
                 && (fileFilter == nullptr || fileFilter->isDirectorySuitable (f));

    return (flags & canSelectFiles) != 0 && f.exists()
             && (fileFilter == nullptr || fileFilter->isFileSuitable (f));
}

//==============================================================================
const File& FileBrowserComponent::getRoot() const
{
    return currentRoot;
}

void FileBrowserComponent::setRoot (const File& newRootDirectory)
{
    bool callListeners = false;

    if (currentRoot != newRootDirectory)
    {
        callListeners = true;
        fileListComponent->scrollToTop();

        String path (newRootDirectory.getFullPathName());

        if (path.isEmpty())
            path = File::getSeparatorString();

        StringArray rootNames, rootPaths;
        getRoots (rootNames, rootPaths);

        if (! rootPaths.contains (path, true))
        {
            bool alreadyListed = false;

            for (int i = currentPathBox.getNumItems(); --i >= 0;)
            {
                if (currentPathBox.getItemText (i).equalsIgnoreCase (path))
                {
                    alreadyListed = true;
                    break;
                }
            }

            if (! alreadyListed)
                currentPathBox.addItem (path, currentPathBox.getNumItems() + 1);
        }
    }

    currentRoot = newRootDirectory;
    fileList->setDirectory (currentRoot, true, true);

    if (auto* tree = dynamic_cast<FileTreeComponent*> (fileListComponent.get()))
        tree->refresh();

    juce::String currentRootName = currentRoot.getFullPathName();

    if (currentRootName.isEmpty())
        currentRootName = File::getSeparatorString();

    currentPathBox.setText (currentRootName, dontSendNotification);

    goUpButton->setEnabled (currentRoot.getParentDirectory().isDirectory()
                             && currentRoot.getParentDirectory() != currentRoot);

    if (callListeners)
    {
        Component::BailOutChecker checker (this);
        listeners.callChecked (checker, [&] (FileBrowserListener& l) { l.browserRootChanged (currentRoot); });
    }
}

void FileBrowserComponent::resetRecentPaths()
{
    currentPathBox.clear();

    StringArray rootNames, rootPaths;
    getRoots (rootNames, rootPaths);

    int nSeparators = 0;
    for (int i = 0; i < rootNames.size(); ++i)
    {
        if (rootNames[i].isEmpty())
        {
            currentPathBox.addSeparator();
            nSeparators++;
        }
        else
        {
            currentPathBox.addItem(rootNames[i], i + 1 - nSeparators);
        }
    }

    currentPathBox.addSeparator();
}

void FileBrowserComponent::goUp()
{
    setRoot (getRoot().getParentDirectory());
}

void FileBrowserComponent::refresh()
{
    fileList->refresh();
}

void FileBrowserComponent::setFileFilter (const FileFilter* const newFileFilter)
{
    if (fileFilter != newFileFilter)
    {
        fileFilter = newFileFilter;
        refresh();
    }
}

String FileBrowserComponent::getActionVerb() const
{
    return isSaveMode() ? ((flags & canSelectDirectories) != 0 ? TRANS ("Choose")
                                                               : TRANS ("Save"))
                        : TRANS ("Open");
}

FilePreviewComponent* FileBrowserComponent::getPreviewComponent() const noexcept
{
    return previewComp;
}

DirectoryContentsDisplayComponent* FileBrowserComponent::getDisplayComponent() const noexcept
{
    return fileListComponent.get();
}

//==============================================================================
void FileBrowserComponent::resized()
{
    layoutFileBrowserComponent();
}

//copied from LookAndFeel_V4
void FileBrowserComponent::layoutFileBrowserComponent()
{
    auto sectionHeight = 22;
    auto buttonWidth = 44;
    int spaceBetweenButtons = 6;
    auto b = this->getLocalBounds().reduced(20, 5);

    auto topSlice = b.removeFromTop(sectionHeight);
    //auto bottomSlice = b.removeFromBottom(sectionHeight);

    currentPathBox.setBounds(topSlice.removeFromLeft(topSlice.getWidth() - 2*(buttonWidth + spaceBetweenButtons)));

    topSlice.removeFromLeft(spaceBetweenButtons);
    musicLibButton.get()->setBounds(topSlice.removeFromLeft(buttonWidth));

    topSlice.removeFromLeft(spaceBetweenButtons);
    goUpButton.get()->setBounds(topSlice);

    if (previewComp != nullptr)
        previewComp->setBounds(b.removeFromRight(b.getWidth() / 3));

    if (auto* listAsComp = dynamic_cast<Component*> (fileListComponent.get()))
        listAsComp->setBounds(b.reduced(0, 10));
}

//==============================================================================
void FileBrowserComponent::lookAndFeelChanged()
{
    goUpButton.reset(myLookAndFeel->createFileBrowserGoUpButton());
    musicLibButton.reset(myLookAndFeel->createFileBrowserMusicLibButton());


    if (auto* buttonPtr = goUpButton.get())
    {
        addAndMakeVisible (*buttonPtr);
        buttonPtr->onClick = [this] { goUp(); };
        buttonPtr->setTooltip(juce::translate("Go up to parent directory"));
    }

    if (auto* buttonPtr = musicLibButton.get())
    {
        addAndMakeVisible(*buttonPtr);
        buttonPtr->onClick = [this] { if (OnMusicLibButtonClick) { OnMusicLibButtonClick(); }};
        buttonPtr->setTooltip(juce::translate("(Un-)Sets current directory as Music Library"));
    }

    currentPathBox.setColour (ComboBox::backgroundColourId,    findColour (currentPathBoxBackgroundColourId));
    currentPathBox.setColour (ComboBox::textColourId,          findColour (currentPathBoxTextColourId));
    currentPathBox.setColour (ComboBox::arrowColourId,         findColour (currentPathBoxArrowColourId));

    resized();

    Component::BailOutChecker checker(this);
    listeners.callChecked(checker, [&](FileBrowserListener& l) { l.browserRootChanged(currentRoot); });

}

//==============================================================================
void FileBrowserComponent::sendListenerChangeMessage()
{
    Component::BailOutChecker checker (this);

    if (previewComp != nullptr)
        previewComp->selectedFileChanged (getSelectedFile (0));

    // You shouldn't delete the browser when the file gets changed!
    jassert (! checker.shouldBailOut());

    listeners.callChecked (checker, [] (FileBrowserListener& l) { l.selectionChanged(); });
}

void FileBrowserComponent::selectionChanged()
{
    StringArray newFilenames;
    bool resetChosenFiles = true;

    for (int i = 0; i < fileListComponent->getNumSelectedFiles(); ++i)
    {
        const File f (fileListComponent->getSelectedFile (i));

        if (isFileOrDirSuitable (f))
        {
            if (resetChosenFiles)
            {
                chosenFiles.clear();
                resetChosenFiles = false;
            }

            chosenFiles.add (f);
            newFilenames.add (f.getRelativePathFrom (getRoot()));
        }
    }

    //if (newFilenames.size() > 0)
    //    filenameBox.setText (newFilenames.joinIntoString (", "), false);

    sendListenerChangeMessage();
}

void FileBrowserComponent::fileClicked (const File& f, const MouseEvent& e)
{
    Component::BailOutChecker checker (this);
    listeners.callChecked (checker, [&] (FileBrowserListener& l) { l.fileClicked (f, e); });
}

void FileBrowserComponent::fileDoubleClicked (const File& f)
{
    if (f.isDirectory())
    {
        setRoot (f);

        //if ((flags & canSelectDirectories) != 0 && (flags & doNotClearFileNameOnRootChange) == 0)
        //    filenameBox.setText ({});
    }
    else
    {
        Component::BailOutChecker checker (this);
        listeners.callChecked (checker, [&] (FileBrowserListener& l) { l.fileDoubleClicked (f); });
    }
}

void FileBrowserComponent::browserRootChanged (const File&) {}

bool FileBrowserComponent::keyPressed ([[maybe_unused]] const KeyPress& key)
{
   #if JUCE_LINUX || JUCE_BSD || JUCE_WINDOWS
    if (key.getModifiers().isCommandDown()
         && (key.getKeyCode() == 'H' || key.getKeyCode() == 'h'))
    {
        fileList->setIgnoresHiddenFiles (! fileList->ignoresHiddenFiles());
        fileList->refresh();
        return true;
    }
   #endif

    return false;
}

//==============================================================================
void FileBrowserComponent::updateSelectedPath()
{
    auto newText = currentPathBox.getText().trim().unquoted();

    if (newText.isNotEmpty())
    {
        auto index = currentPathBox.getSelectedId() - 1;

        StringArray rootNames, rootPaths;
        getRoots (rootNames, rootPaths);

        if (rootPaths[index].isNotEmpty())
        {
            setRoot (File (rootPaths[index]));
        }
        else
        {
            File f (newText);

            for (;;)
            {
                if (f.isDirectory())
                {
                    setRoot (f);
                    break;
                }

                if (f.getParentDirectory() == f)
                    break;

                f = f.getParentDirectory();
            }
        }
    }
}

bool FileBrowserComponent::isMusicLibButtonVisible()
{

    if (myLookAndFeel != nullptr)
    {
        return true;
    }

    return false;
}

void FileBrowserComponent::getDefaultRoots (StringArray& rootNames, StringArray& rootPaths)
{
   #if JUCE_WINDOWS
    Array<File> roots;
    File::findFileSystemRoots (roots);
    rootPaths.clear();

    for (int i = 0; i < roots.size(); ++i)
    {
        const File& drive = roots.getReference (i);

        String name (drive.getFullPathName());
        rootPaths.add (name);

        if (drive.isOnHardDisk())
        {
            String volume (drive.getVolumeLabel());

            if (volume.isEmpty())
                volume = TRANS ("Hard Drive");

            name << " [" << volume << ']';
        }
        else if (drive.isOnCDRomDrive())
        {
            name << " [" << TRANS ("CD/DVD drive") << ']';
        }

        rootNames.add (name);
    }

    rootPaths.add ({});
    rootNames.add ({});

    rootPaths.add (File::getSpecialLocation (File::userDocumentsDirectory).getFullPathName());
    rootNames.add (TRANS ("Documents"));
    rootPaths.add (File::getSpecialLocation (File::userMusicDirectory).getFullPathName());
    rootNames.add (TRANS ("Music"));
    rootPaths.add (File::getSpecialLocation (File::userPicturesDirectory).getFullPathName());
    rootNames.add (TRANS ("Pictures"));
    rootPaths.add (File::getSpecialLocation (File::userDesktopDirectory).getFullPathName());
    rootNames.add (TRANS ("Desktop"));

   #elif JUCE_MAC
    rootPaths.add (File::getSpecialLocation (File::userHomeDirectory).getFullPathName());
    rootNames.add (TRANS ("Home folder"));
    rootPaths.add (File::getSpecialLocation (File::userDocumentsDirectory).getFullPathName());
    rootNames.add (TRANS ("Documents"));
    rootPaths.add (File::getSpecialLocation (File::userMusicDirectory).getFullPathName());
    rootNames.add (TRANS ("Music"));
    rootPaths.add (File::getSpecialLocation (File::userPicturesDirectory).getFullPathName());
    rootNames.add (TRANS ("Pictures"));
    rootPaths.add (File::getSpecialLocation (File::userDesktopDirectory).getFullPathName());
    rootNames.add (TRANS ("Desktop"));

    rootPaths.add ({});
    rootNames.add ({});

    for (auto& volume : File ("/Volumes").findChildFiles (File::findDirectories, false))
    {
        if (volume.isDirectory() && ! volume.getFileName().startsWithChar ('.'))
        {
            rootPaths.add (volume.getFullPathName());
            rootNames.add (volume.getFileName());
        }
    }

   #else
    rootPaths.add ("/");
    rootNames.add ("/");
    rootPaths.add (File::getSpecialLocation (File::userHomeDirectory).getFullPathName());
    rootNames.add (TRANS ("Home folder"));
    rootPaths.add (File::getSpecialLocation (File::userDesktopDirectory).getFullPathName());
    rootNames.add (TRANS ("Desktop"));
   #endif
}

void FileBrowserComponent::getRoots (StringArray& rootNames, StringArray& rootPaths)
{
    getDefaultRoots (rootNames, rootPaths);
}

void FileBrowserComponent::timerCallback()
{
    const auto isProcessActive = juce::detail::WindowingHelpers::isForegroundOrEmbeddedProcess (this);
    
    if (wasProcessActive != isProcessActive)
    {
        wasProcessActive = isProcessActive;

        if (isProcessActive && fileList != nullptr)
            refresh();
    }
}

//==============================================================================
std::unique_ptr<AccessibilityHandler> FileBrowserComponent::createAccessibilityHandler()
{
    return std::make_unique<AccessibilityHandler> (*this, AccessibilityRole::group);
}

void FileBrowserComponent::setMusicLibState(bool isCurrentlyMusicLib)
{
    musicLibButton->setToggleState(isCurrentlyMusicLib, juce::NotificationType::dontSendNotification);

}











} // namespace juce::mod
