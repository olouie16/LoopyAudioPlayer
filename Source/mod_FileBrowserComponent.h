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

#pragma once

#include <JuceHeader.h>
#include "OwnLookAndFeel.h"

namespace juce::mod
{

//==============================================================================
/**
    A component for browsing and selecting a file or directory to open or save.

    This contains a FileListComponent and adds various boxes and controls for
    navigating and selecting a file. It can work in different modes so that it can
    be used for loading or saving a file, or for choosing a directory.

    @see FileChooserDialogBox, FileChooser, FileListComponent

    @tags{GUI}
*/
class JUCE_API  FileBrowserComponent  : public Component,
                                        private FileBrowserListener,
                                        private FileFilter,
                                        private Timer
{
public:
    //==============================================================================
    /** Various options for the browser.

        A combination of these is passed into the FileBrowserComponent constructor.
    */
    enum FileChooserFlags
    {
        openMode                        = 1,    /**< specifies that the component should allow the user to
                                                     choose an existing file with the intention of opening it. */
        saveMode                        = 2,    /**< specifies that the component should allow the user to specify
                                                     the name of a file that will be used to save something. */
        canSelectFiles                  = 4,    /**< specifies that the user can select files (can be used in
                                                     conjunction with canSelectDirectories). */
        canSelectDirectories            = 8,    /**< specifies that the user can select directories (can be used in
                                                     conjunction with canSelectFiles). */
        canSelectMultipleItems          = 16,   /**< specifies that the user can select multiple items. */
        useTreeView                     = 32,   /**< specifies that a tree-view should be shown instead of a file list. */
        filenameBoxIsReadOnly           = 64,   /**< specifies that the user can't type directly into the filename box. */
        warnAboutOverwriting            = 128,  /**< specifies that the dialog should warn about overwriting existing files (if possible). */
        doNotClearFileNameOnRootChange  = 256   /**< specifies that the file name should not be cleared upon root change. */
    };

    //==============================================================================
    /** Creates a FileBrowserComponent.

        @param flags                    A combination of flags from the FileChooserFlags enumeration, used to
                                        specify the component's behaviour. The flags must contain either openMode
                                        or saveMode, and canSelectFiles and/or canSelectDirectories.
        @param initialFileOrDirectory   The file or directory that should be selected when the component begins.
                                        If this is File(), a default directory will be chosen.
        @param fileFilter               an optional filter to use to determine which files are shown.
                                        If this is nullptr then all files are displayed. Note that a pointer
                                        is kept internally to this object, so make sure that it is not deleted
                                        before the FileBrowserComponent object is deleted.
        @param previewComp              an optional preview component that will be used to show previews of
                                        files that the user selects
    */
    FileBrowserComponent (int flags,
                          const File& initialFileOrDirectory,
                          const FileFilter* fileFilter,
                          FilePreviewComponent* previewComp,
                          OwnLookAndFeel* myLookAndFeel);

    /** Destructor. */
    ~FileBrowserComponent() override;



    //==============================================================================
    /** Returns the number of files that the user has got selected.
        If multiple select isn't active, this will only be 0 or 1. To get the complete
        list of files they've chosen, pass an index to getCurrentFile().
    */
    int getNumSelectedFiles() const noexcept;

    /** Returns one of the files that the user has chosen.
        If the box has multi-select enabled, the index lets you specify which of the files
        to get - see getNumSelectedFiles() to find out how many files were chosen.
        @see getHighlightedFile
    */
    File getSelectedFile (int index) const noexcept;

    /** Deselects any files that are currently selected. */
    void deselectAllFiles();

    /** Returns true if the currently selected file(s) are usable.

        This can be used to decide whether the user can press "ok" for the
        current file. What it does depends on the mode, so for example in an "open"
        mode, this only returns true if a file has been selected and if it exists.
        In a "save" mode, a non-existent file would also be valid.
    */
    bool currentFileIsValid() const;

    /** This returns the last item in the view that the user has highlighted.
        This may be different from getCurrentFile(), which returns the value
        that is shown in the filename box, and if there are multiple selections,
        this will only return one of them.
        @see getSelectedFile
    */
    File getHighlightedFile() const noexcept;

    //==============================================================================
    /** Returns the directory whose contents are currently being shown in the listbox. */
    const File& getRoot() const;

    /** Changes the directory that's being shown in the listbox. */
    void setRoot (const File& newRootDirectory);

    /** Changes the name that is currently shown in the filename box. */
    //void setFileName (const String& newName);

    /** Equivalent to pressing the "up" button to browse the parent directory. */
    void goUp();

    /** Refreshes the directory that's currently being listed. */
    void refresh();

    /** Changes the filter that's being used to sift the files. */
    void setFileFilter (const FileFilter* newFileFilter);

    /** Returns a verb to describe what should happen when the file is accepted.

        E.g. if browsing in "load file" mode, this will be "Open", if in "save file"
        mode, it'll be "Save", etc.
    */
    virtual String getActionVerb() const;

    /** Returns true if the saveMode flag was set when this component was created. */
    bool isSaveMode() const noexcept;

    //==============================================================================
    /** Adds a listener to be told when the user selects and clicks on files.
        @see removeListener
    */
    void addListener (FileBrowserListener* listener);

    /** Removes a listener.
        @see addListener
    */
    void removeListener (FileBrowserListener* listener);

    /** Returns a platform-specific list of names and paths for some suggested places the user
        might want to use as root folders.
        The list returned contains empty strings to indicate section breaks.
        @see getRoots()
    */
    static void getDefaultRoots (StringArray& rootNames, StringArray& rootPaths);

    //==============================================================================
    /** This abstract base class is implemented by LookAndFeel classes to provide
        various file-browser layout and drawing methods.
    */
    struct JUCE_API  LookAndFeelMethods
    {
        virtual ~LookAndFeelMethods() = default;

        // These return a pointer to an internally cached drawable - make sure you don't keep
        // a copy of this pointer anywhere, as it may become invalid in the future.
        virtual const Drawable* getDefaultFolderImage() = 0;
        virtual const Drawable* getDefaultDocumentFileImage() = 0;

        virtual AttributedString createFileChooserHeaderText (const String& title,
                                                              const String& instructions) = 0;

        virtual void drawFileBrowserRow (Graphics&, int width, int height,
                                         const File& file,
                                         const String& filename,
                                         Image* optionalIcon,
                                         const String& fileSizeDescription,
                                         const String& fileTimeDescription,
                                         bool isDirectory,
                                         bool isItemSelected,
                                         int itemIndex,
                                         DirectoryContentsDisplayComponent&) = 0;

        virtual Button* createFileBrowserGoUpButton() = 0;

        virtual void layoutFileBrowserComponent (FileBrowserComponent& browserComp,
                                                 DirectoryContentsDisplayComponent* fileListComponent,
                                                 FilePreviewComponent* previewComp,
                                                 ComboBox* currentPathBox,
                                                 TextEditor* filenameBox,
                                                 Button* goUpButton) = 0;
    };

    /** A set of colour IDs to use to change the colour of various aspects of the FileBrowserComponent.

        These constants can be used either via the Component::setColour(), or LookAndFeel::setColour()
        methods.

        @see Component::setColour, Component::findColour, LookAndFeel::setColour, LookAndFeel::findColour
    */
    enum ColourIds
    {
        currentPathBoxBackgroundColourId    = 0x1000640, /**< The colour to use to fill the background of the current path ComboBox. */
        currentPathBoxTextColourId          = 0x1000641, /**< The colour to use for the text of the current path ComboBox. */
        currentPathBoxArrowColourId         = 0x1000642, /**< The colour to use to draw the arrow of the current path ComboBox. */
        filenameBoxBackgroundColourId       = 0x1000643, /**< The colour to use to fill the background of the filename TextEditor. */
        filenameBoxTextColourId             = 0x1000644  /**< The colour to use for the text of the filename TextEditor. */
    };

    //==============================================================================
    /** @internal */
    void resized() override;
    void layoutFileBrowserComponent();
    /** @internal */
    void lookAndFeelChanged() override;
    /** @internal */
    bool keyPressed (const KeyPress&) override;
    /** @internal */
    void selectionChanged() override;
    /** @internal */
    void fileClicked (const File&, const MouseEvent&) override;
    /** @internal */
    void fileDoubleClicked (const File&) override;
    /** @internal */
    void browserRootChanged (const File&) override;
    /** @internal */
    bool isFileSuitable (const File&) const override;
    /** @internal */
    bool isDirectorySuitable (const File&) const override;
    /** @internal */
    FilePreviewComponent* getPreviewComponent() const noexcept;
    /** @internal */
    DirectoryContentsDisplayComponent* getDisplayComponent() const noexcept;
    /** @internal */
    std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override;

    void setMusicLibState(bool isCurrentlyMusicLib);

    std::function<void()> OnMusicLibButtonClick;

protected:
    /** Returns a list of names and paths for the default places the user might want to look.

        By default this just calls getDefaultRoots(), but you may want to override it to
        return a custom list.
    */
    virtual void getRoots (StringArray& rootNames, StringArray& rootPaths);

    /** Updates the items in the dropdown list of recent paths with the values from getRoots(). */
    void resetRecentPaths();

private:
    //==============================================================================
    std::unique_ptr<DirectoryContentsList> fileList;
    const FileFilter* fileFilter;

    int flags;
    File currentRoot;
    Array<File> chosenFiles;
    ListenerList<FileBrowserListener> listeners;

    std::unique_ptr<DirectoryContentsDisplayComponent> fileListComponent;
    FilePreviewComponent* previewComp;
    ComboBox currentPathBox;
    std::unique_ptr<Button> goUpButton;
    std::unique_ptr<Button> musicLibButton;
    TimeSliceThread thread;
    bool wasProcessActive;

    OwnLookAndFeel* myLookAndFeel;

    void timerCallback() override;
    void sendListenerChangeMessage();
    bool isFileOrDirSuitable (const File&) const;
    void updateSelectedPath();
    bool isMusicLibButtonVisible();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileBrowserComponent)
};










} // namespace juce
